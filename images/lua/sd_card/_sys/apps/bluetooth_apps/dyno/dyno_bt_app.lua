app.name = "Dyno BT"
app.id = "dynawa.dyno"

function app:start()
	self.events = Class.EventSource("watch")
	self.activities = {}
	local mine = self:load_data()
	if mine then
		mine = assert(mine.devices)
		for bdaddr, device in pairs(mine) do
			self:new_activity(bdaddr,assert(device.status))
		end
	end
	dynawa.bluetooth_manager.events:register_for_events(self)
	self:status_changed()
end

--Creates 'activity' structure. The bdaddr device must already be paired with the watch.
function app:new_activity(bdaddr, status)
	local device = dynawa.bluetooth_manager.prefs.devices[bdaddr]
	if not device then
		return nil, "This device is not paired with the watch"
	end
	local act = {id = "act"..dynawa.unique_id()}
	act.name = device.name
	act.bdaddr = bdaddr
	act.status = status or "bt_off"
	self.activities[act.id] = act
	if dynawa.bluetooth_manager.hw_status == "on" then
		self:activity_start(act)
	end
	return act
end

function app:try_activity(badmac)
	--Used for trying different activities in case of failure.
	--It makes a sorted list of all CONNECTED activities
	--The order is determined by alphabetically sorting the activities based on the string concat of MAC address and activity id.
	--Returns either the first activity, or - if badmac is specified - the first activity AFTER the activity with MAC equal to "badmac".
	--If badmac value is higher than MAC of the last activity, returns the FIRST activity instead.
	--If only single activity is available, returns this activity regardless of badmac value
	--If no activities are available, returns nil
	local acts = {}
	for k,act in pairs(self.activities) do
		if act.status == "connected" then
			table.insert(acts,act)
		end
	end
	if #acts == 0 then
		return nil
	end
	if #acts == 1 then
		return acts[1]
	end
	table.sort(acts, function (a,b) return (a.bdaddr..a.id < b.bdaddr..b.id) end)
	if not badmac or badmac >= acts[#acts].bdaddr then
		return acts[1]
	end
	while badmac <= acts[1].bdaddr do
		local act = table.remove(acts,1)
		table.insert(acts,act)
	end
	return acts[1]
end

local function to_word(num) --Convert integer to 2 byte word
	return string.char(math.floor(num / 256))..string.char(num%256)
end

local function from_word(bytes) --Convert 2 byte word to int
	return (bytes:sub(1)):byte() * 256 + (bytes:sub(2)):byte()
end

local function safe_string(str)
	local result = {}
	for chr in str:gmatch(".") do
		if chr >=" " and chr <= "~" then 
			table.insert(result,chr)
		else
			table.insert(result,string.format("\\%03d",string.byte(chr)))
		end
	end
	return table.concat(result)
end

function app:handle_bt_event_turned_on()
	local mine = self:load_data()
	if mine then
		mine = assert(mine.devices)
	else
		mine = {}
	end
	for act_id,act in pairs(self.activities) do
		if mine[act.bdaddr].status == "disabled" then
			act.status = "disabled"
		end
		self:activity_start(act)
	end
end

function app:handle_bt_event_turning_off()
	for id, activity in pairs(self.activities) do
		self:activity_stop(activity)
		activity.status = "bt_off"
	end
end

function app:activity_start(act)
	assert(act)
	--log("act.status = "..tostring(act.status))
	if act.status == "disabled" then
		return nil
	end
	local socket = assert(self:new_socket("sdp"))
	act.socket = socket
	socket.activity = act
	assert(socket._c)
	assert(act.bdaddr)
	act.channel = false
	act.status = "finding_service"
	log("Connecting to "..act.name)
    dynawa.devices.bluetooth.cmd:find_service(socket._c, act.bdaddr)
    self:status_changed()
    return act
end

function app:activity_enable(activity)
	assert(activity.status == "disabled", "Activity status is not 'disabled' but "..tostring(activity.status))
	activity.status = "bt_off"
	self:save_prefs()
	if dynawa.bluetooth_manager.hw_status == "on" then
		self:activity_start(activity)
	end
end

function app:activity_stop(activity)
	if activity.socket then
		if not activity.socket.__deleted then
			log("Trying to close "..activity.socket)
			activity.socket:close()
		end
		activity.socket = nil
	end
	activity.sender = nil
	activity.reconnect_delay = nil
	activity.status = nil
	self:status_changed()
end

function app:activity_disable(activity)
	self:activity_stop(activity)
	activity.status = "disabled"
	self:save_prefs()
end

function app:handle_event_removed_paired_device(event)
	local device = assert(event.device)
	assert(device.bdaddr)
	--Does this device concern us?
	for id,act in pairs(self.activities) do
		if assert(act.bdaddr) == device.bdaddr then
			self:activity_stop(act)
			assert(self.activities[act.id],"Activity vanished too soon")
			self.activities[act.id] = nil
			self:save_prefs()
			return
		end
	end
end

function app:handle_event_socket_data(socket, data_in)
	assert(data_in)
	local activity = assert(socket.activity)
	--log("Got "..#data_in.." bytes of data: "..safe_string(data_in))
	
	--[[
	while #data_in > 2 and (string.byte(data_in) < 180 or string.byte(data_in) > 183) do
		--Got some non-packet data, skip it
		data_in = data_in:sub(2)
	end
	--]]

	if socket.partial_chunk then
		data_in = socket.partial_chunk .. data_in
		socket.partial_chunk = nil
	end

	local len1,len2,body = data_in:match("(.)(.)(.*)")
	assert(data_in, "Header mismatch")
	local len = (len1:byte() % 4) * 256 + len2:byte()
	assert(len > 0, "Zero length")
	if len > #body then
		--log("*** Partial chunk size = "..(#data_in - 2))
		socket.partial_chunk = data_in
		return
	end
	--log("Chunk complete, header indicates size = "..len)
	assert(len == #body, string.format("Chunk size is %s but should be %s according to its header.", #body, len))
	self:activity_chunk_received(activity, body)
end

function app:activity_chunk_received(activity, chunk)
	if not activity.socket then --Closed before the chunk was received
		return
	end
	local socket = activity.socket
	--log("Chunk is "..#chunk.." bytes")
	local file_id, piece_n_str, of_str, piece = chunk:match("^P(...)(..)(..)(.*)$")
	if piece then --It's P chunk
		local piece_n = from_word(piece_n_str)
		local of = from_word(of_str)
		if piece_n == 1 then
			activity.receiver = {pieces = {piece}, file_id = file_id}
		else
			assert(activity.receiver.file_id == file_id, "Mismatched file_id for piece "..piece_n..", expected "..safe_string(activity.receiver.file_id)..", got "..safe_string(file_id))
			table.insert(activity.receiver.pieces, piece)
			assert(#activity.receiver.pieces == piece_n, "Wrong piece_n: "..piece_n)
		end
		--log("Acknowledging piece "..piece_n.." of "..of..", file id "..safe_string(file_id))
		local ack = table.concat({"A",file_id,piece_n_str})
		self:activity_send_chunk(activity, ack)
		if piece_n == of then --binstring is complete
			local binstring = table.concat(activity.receiver.pieces)
			activity.receiver = nil
			self:activity_got_binstring(activity, binstring)
		end
		return
	end
	assert(chunk:match("^A.....$"), "Not Ack chunk")
	if not activity.sender or chunk ~= activity.sender.waiting_for_ack then
		log("Unexpected Ack chunk received (ignored)")
		return
	end
	--log("Ack OK")
	activity.sender.waiting_for_ack = nil
	if #activity.sender.pieces > 0 then
		self:activity_send_piece(activity)
	end
end

function app:activity_got_binstring(activity, binstring)
	--log("Parsing binstring: "..safe_string(binstring))
	local value, rest = self:binstring_to_value(binstring)
	assert (rest == "", #rest.." unconsumed bytes after binstring parsing")
	--log("Parsed result: OK")
	--log("PARSED RESULT VALUE: "..dynawa.file.serialize(value))
	if type(value) == "table" and value.command then
		if value.command == "echo" then
			log("Echoing back...")
			self:activity_send_data(activity,assert(value.data))
		elseif value.command == "time_sync" then
			local time = assert(value.time)
			local t0 = os.time()
			log (string.format("Time sync: %+d seconds",time - t0))
			dynawa.time.set(time)
			self:status_changed()
		else
			value.bdaddr = assert(activity.bdaddr)
			log("-----Dyno incoming event: "..dynawa.file.serialize(value))
			self.events:generate_event{type = "dyno_data_from_phone", data = value}
		end
	end
end

function app:binstring_to_value(binstring)
	local typ = binstring:sub(1,1)
	if typ == "T" then
		return true, binstring:sub(2)
	elseif typ == "F" then
		return false, binstring:sub(2)
	elseif typ == "$" then
		local len, rest = binstring:match("^%$(%d*):(.*)$")
		len = assert(tonumber(len), "String length is nil")
		return rest:sub(1,len), rest:sub(len+1)
	elseif typ == "#" then
		local num,rest = binstring:match("^#(.-);(.*)$")
		num = tonumber(num)
		assert(num, "Expected a number but parsed string to nil")
		return num, rest
	elseif typ == "@" then
		local array = {}
		local rest = binstring:sub(2)
		local item
		while true do
			if rest:sub(1,1) == ";" then
				return array, rest:sub(2)
			end
			item, rest = self:binstring_to_value(rest)
			table.insert(array, item)
		end
		error("WTF?")
	elseif typ == "*" then
		local hash = {}
		local rest = binstring:sub(2)
		local key, val
		while true do
			if rest:sub(1,1) == ";" then
				return hash, rest:sub(2)
			end
			key, rest = self:binstring_to_value(rest)
			assert(type(key)=="string", "Hash key is not string but "..tostring(key))
			val, rest = self:binstring_to_value(rest)
			hash[key] = val
		end
		error("WTF?")
	else
		error("Unknown value type: "..typ)
	end
end

function app:handle_event_socket_connected(socket)
	log(self.." socket connected: "..socket)
	socket.activity.status = "connected"
	socket.activity.reconnect_delay = nil
	self:info("Succesfully connected to "..socket.activity.name)
	self:activity_send_data(socket.activity,"HELLO")
	self:status_changed()
end

function app:send_data_test(data)
	local id, activity = next(self.activities)
	if not id then
		dynawa.popup:error("Not connected (no Activity)")
		return
	end
	if activity.status ~= "connected" then
		dynawa.popup:error("Not connected - Activity status is '"..tostring(activity.status).."'")
		return
	end
	self:activity_send_data(activity, data)
end

-- Sends data to given activity
function app:activity_send_data(activity, data)
	--log("---Sending data from watch")
	data = table.concat(self:encode_data(data, {}))
	if activity.status ~= "connected" then
		return false, "Not connected - Activity status is '"..tostring(activity.status).."'"
	end
	if not activity.sender then
		activity.sender = {pieces={}}
	end
	self:split_data(data, 100, activity.sender.pieces)
	if not activity.sender.waiting_for_ack then
		self:activity_send_piece(activity)
	else
		--return false, "Cannot send piece - still waiting for Ack chunk for previously sent piece"
	end
	return true
end

-- Sends data to given phone
function app:bdaddr_send_data(bdaddr, data)
	for id,act in pairs(self.activities) do
		if act.bdaddr == bdaddr then
			return self:activity_send_data(act,data)
		end
	end
	return false, "Unknown phone"
end

function app:encode_data(data, parts)
	assert(parts)
	local typ = type(data)
	if typ == "string" then
		table.insert(parts,"$"..#data..":")
		table.insert(parts,data)
	elseif typ == "boolean" then
		if data then
			table.insert(parts,"T")
		else
			table.insert(parts,"F")
		end
	elseif typ == "number" then
		table.insert(parts, "#"..data..";")
	elseif typ == "table" then
		if type(next(data)) == "number" then --It's an array
			table.insert(parts,"@")
			for i,elem in ipairs(data) do
				self:encode_data(elem,parts)
			end
		else --It's hash table
			table.insert(parts,"*")
			for key,elem in pairs(data) do
				assert(type(key) == "string", "Hash table key is not string")
				self:encode_data(key,parts)
				self:encode_data(elem,parts)
			end
		end
		table.insert(parts,";")
	else
		error("Unable to encode data type: "..typ)
	end
	return parts
end

function app:activity_send_piece(activity)
	assert(activity.sender.pieces)
	local piece = assert(table.remove(activity.sender.pieces, 1))
	--log("Sending piece #"..string.byte(piece:sub(6)).." of "..string.byte(piece:sub(8)))
	activity.sender.waiting_for_ack = "A"..piece:sub(2,6)
	self:activity_send_chunk(activity, piece)
end

function app:split_data(data, piece_size, pieces)
	local file_id = string.char(math.random(256)-1)..string.char(math.random(256)-1)..string.char(math.random(256)-1)
	local size = #data
	assert(size > 0, "Empty data")
	local n_pieces = math.floor(size / piece_size) + 1
	--log("Split to "..n_pieces.." pieces...")
	for n_piece = 1,n_pieces do
		local f, t = (n_piece - 1) * piece_size + 1, n_piece * piece_size
		local substr = data:sub(f,t)
		substr = table.concat({"P",file_id,to_word(n_piece),to_word(n_pieces),substr})
		--log(string.format("Piece %s: %s",n_piece, safe_string(substr)))
		table.insert(pieces,substr)
	end
	return pieces
end

function app:activity_send_chunk(activity, chunk)
	assert (#chunk <= 1023 and #chunk > 0)
	local header = to_word((180*256) + #chunk)
	local data = header..chunk
	--log("Sending chunk with header: "..safe_string(data))
	assert(activity.socket):send(data)
end

function app:handle_event_socket_disconnected(socket,prev_state)
	log(socket.." disconnected")
	local activity = socket.activity
	if prev_state == "connected" then
		self:info("Disconnected from "..activity.name)
	end
	self.events:generate_event{type="dyno_device_disconnected",bdaddr = assert(activity.bdaddr)}
	activity.socket = nil
	socket:_delete()
	self:should_reconnect(activity)
end

function app:should_reconnect(activity)
	assert(not activity.__deleted)
	activity.status = "waiting_for_reconnect"
	activity.reconnect_delay = math.min((activity.reconnect_delay or 1000) * 2, 600000) --10 minutes max.
	log("Waiting "..activity.reconnect_delay.." ms before trying to reconnect "..activity.name)
	dynawa.devices.timers:timed_event{delay = activity.reconnect_delay, receiver = self, what = "attempt_reconnect", activity = activity}
	self:status_changed()
end

function app:handle_event_timed_event(message)
	assert(message.what == "attempt_reconnect")
	local activity = assert(message.activity)
	if activity.__deleted then
		return
	end
	if activity.status ~= "waiting_for_reconnect" then
		return
	end
	log("Trying to reconnect "..activity.name)
	self:activity_start(activity)
end

function app:handle_event_socket_error(socket,error)
	log("BT error '"..tostring(error).."' in "..socket)
	--#todo destroy the socket!
	local activity = assert(socket.activity)
	self:should_reconnect(activity)
end

function app:handle_event_socket_find_service_result(sock0,channel)
	log ("Find_service_result channel = "..tostring(channel))
	local activity = sock0.activity
	if channel == 0 then
		self:should_reconnect(activity)
		return
		-- android donut dyno workaround
		-- channel = 15
	end
	activity.channel = channel
	local socket = self:new_socket("rfcomm")
	activity.socket = socket
	socket.activity = activity
	activity.status = "connecting"
	socket:connect(activity.bdaddr, activity.channel)
end

function app:info(txt)
	--[[if self.last_info == txt then
		return
	end
	self.last_info = txt]]
	dynawa.popup:open{text = txt, autoclose = true}
	dynawa.devices.vibrator:alert()
end

function app:handle_event_do_menu()
	local menudesc = {banner = "Dyno's phones:"}
	local items = assert(self:activity_items())
	table.insert(items,{text = "Add another (already paired) phone", selected = function(_self,args)
		local menudesc = {banner = "Choose the phone to add to Dyno:"}
		local mine = self:load_data()
		if mine then
			mine = assert(mine.devices)
		else
			mine = {}
		end
		local items = {}
		for bdaddr,device in pairs(dynawa.bluetooth_manager.prefs.devices) do
			if not mine[bdaddr] then -- This is not one of my devices, offer to add it.
				table.insert(items,{text = device.name, selected = function(_self, args)
					local act, err = self:new_activity(bdaddr)
					if not act then
						dynawa.popup:error(err)
						return
					else
						dynawa.popup:info("Phone added to Dyno")
						self:save_prefs()
					end
				end})
			end
		end
		if not next(items) then
			table.insert(items,{text = "No devices available. Make sure your phone is paired with the watch first."})
		end
		menudesc.items = items
		self:new_menuwindow(menudesc):push()
	end})
	menudesc.items = items
	self:new_menuwindow(menudesc):push()
end

function app:activity_status_text(act)
	--Returns short text and (optionally) color
	local status = assert(act.status)
	if status == "bt_off" then
		return "BT off",{150,150,150}
	end
	if status == "connected" then
		return "connected",{0,255,0}
	end
	if status == "waiting_for_reconnect" then
		return "waiting before reconnect attempt",{255,127,0}
	end
	if status == "disabled" then --#todo
		return "disabled",{150,150,150}
	end
	return status,{255,0,0}
end

function app:activity_menuitem_selected(_self,args)
	local item = assert(args.item)
	local act_id = assert(item.value.act_id)
	local activity = self.activities[act_id]
	if not activity then
		dynawa.popup:error("Unknown device")
		return
	end
	local menudesc = {banner = activity.name, items = {}}
	local text, color = self:activity_status_text(activity)
	local act_id = assert(activity.id)
	table.insert(menudesc.items,{text = "Status: "..text, textcolor = color})
	if activity.status == "disabled" then
		table.insert(menudesc.items, {text = "Enable this phone", selected = function(__self, args)
			if self.activities[act_id] then
				self:activity_enable(activity)
				dynawa.popup:info("Enabled")
			else
				dynawa.popup:error("Unknown device")
			end			
		end})
	else
		table.insert(menudesc.items, {text = "Disable temporarily", selected = function(__self, args)
			if self.activities[act_id] then
				self:activity_disable(activity)
				dynawa.popup:info("Disabled")
			else
				dynawa.popup:error("Unknown device")
			end			
		end})
	end
	table.insert(menudesc.items,{text = "Remove this phone from Dyno", selected = function(__self, args)
		if self.activities[act_id] then
			self:activity_disable(activity)
			self.activities[act_id] = nil
			dynawa.popup:info("Removed")
			self:save_prefs()
		else
			dynawa.popup:error("Unknown device")
		end
	end})
	self:new_menuwindow(menudesc):push()
end

function app:activity_items()
	local items = {}
	for id, act in pairs(self.activities) do
		local stat_txt, color = self:activity_status_text(act)
		local item = {text = act.name.." ("..stat_txt..")", value = {act_id = id}}
		item.selected = function(_self,args)
			self:activity_menuitem_selected(_self,args)
		end
		item.textcolor = color
		table.insert(items,item)
	end
	table.sort(items, function(a,b)
		return (a.text < b.text)
	end)
	return items
end

function app:status_text()
	local num,nconn = 0,0
	for id,act in pairs(self.activities) do
		num = num + 1
		if act.status == "connected" then
			nconn = nconn + 1
		end
	end
	return (nconn.. " out of "..num.." phones connected")
end

function app:save_prefs()
	local prefs = {devices = {}}
	for act_id, activity in pairs(self.activities) do
		local bdaddr = assert(activity.bdaddr)
		assert(not(prefs.devices[bdaddr]))
		local val = {status = "enabled"}
		if activity.status == "disabled" then
			val.status = "disabled"
		end
		prefs.devices[bdaddr] = val
	end
	self:save_data(prefs)
end

function app:status_changed()
	self.events:generate_event{type="dyno_status_changed", activities = self.activities}
end
