app.name = "Default Inbox"
app.id = "dynawa.inbox"

local inbox_names = {
	sms = "SMS messages",
	email = "E-mails",
	calendar = "Calendar",
	call = "Voice calls",
}

local inbox_ids = {"sms","email","calendar","call"}

local highlight_color = {0,226,240}

function app:start()
	local bmap = assert(dynawa.bitmap.from_png_file(self.dir.."gfx.png"))
	self.gfx = {}
	for i,id in ipairs {"email","sms","call","calendar"} do
		self.gfx[id] = dynawa.bitmap.copy(bmap, i*25 - 25, 0, 25, 25)
	end
	self.events = Class.EventSource("inbox")
	self.prefs = self:load_data() or {storage = {email={},sms={},calendar={},call={}}}
--[[	table.insert(self.prefs.storage.email,{header = "We should meet (obama@whitehouse.gov)",
			body={"From: Barack Obama <obama@whitehouse.gov>",{"(Received %s)",os.time() - 300},"Dear Sir,","Please contact me, because we have to meet soon. The future","of the free world is currently at stake","Your truly, Barack"}})

	table.insert(self.prefs.storage.email,{header = "You inherited $80,000,000 congratulations!! (ahmed@niger.net)",
			body={"From: Ahmed Ahmed <ahmed@niger.net>",{"(Received %s)",os.time() - 60 * 60 * 12},"Dear Sir,","Please contact me, because we have to meet soon. You are the sole heir of","the great rich maharaja.","Your truly, Ahmed"}})	

	table.insert(self.prefs.storage.calendar,{header = {"Meet Santa Claus (%s)",os.time() + 60*60*24*7 + 1000},
			body={"At North Pole"}})]]
	local my_events
	dynawa.app_manager:after_app_start("dynawa.dyno",function (dyno)
		dyno.events:register_for_events(self, function(ev)
			if ev.type ~= "dyno_data_from_phone" then
				return false
			end
			local com = ev.data.command
			return (com == "incoming_sms" or com == "incoming_email" or com == "calendar_event") --incoming_call is handled by dynawa.call_monitor
		end)
	end)
	dynawa.app_manager:after_app_start("dynawa.call_manager", function(callman)
		callman.events:register_for_events(self)
	end)
end

--This is normally sent directly from dyno
function app:handle_event_dyno_data_from_phone(ev)
	self:handle_incoming_item(assert(ev.data))
end

--This is normally sent from call_manager
function app:handle_event_incoming_call(ev)
	assert(ev.data.command == "incoming_call")
	self:handle_incoming_item(ev.data)
end

function app:handle_incoming_item(data)
	local icon
	if data.contact_icon then
		icon = assert(data.contact_icon["45"], "No size 45 icon found")
	end
	local command = assert(data.command)
	local item = {body={},read=false}
	local rows = {}
	local typ
	local popup
	if command == "incoming_call" then
		typ = "call"
		if data.duration then
			table.insert(rows,"ACCEPTED CALL")
		else
			table.insert(rows,"MISSED CALL")
		end
		table.insert(rows,data.contact_name or data.contact_phone)
		if data.duration then
			table.insert(rows, "Duration: "..data.duration)
		end
		local caller = data.contact_name
		if not caller then
			caller = assert(data.contact_phone)
		end
		item.header = {"From "..caller.." (%s)",os.time()-5}
		if not data.duration then
			item.header[1] = "MISSED: "..item.header[1]
		end
		if data.contact_name then
			table.insert(item.body,"Number: "..data.contact_phone)
		end
		if data.duration then
			table.insert(item.body,"Call duration: "..data.duration)
			--Accepted call items are automatically marked as "read"
			item.read = true
		end
	elseif command == "incoming_sms" then
		typ = "sms"
		table.insert(rows,"INCOMING SMS")
		local sender = data.contact_name
		if not sender then
			sender = assert(data.contact_phone)
		end
		table.insert(rows,"From: "..sender)
		local snippet = self:get_snippet(data.text)
		table.insert(rows,'"'..snippet..'"')
		local time = assert(data.time_received)
		item.header = {"From ".. sender.. " (%s)",time}
		for i,line in ipairs(data.text) do
			table.insert(item.body,line)
		end
	elseif command == "incoming_email" then
		typ = "email"
		table.insert(rows,"INCOMING E-MAIL")
		local sender = data.contact_name
		if not sender then
			sender = assert(data.contact_email)
		end
		table.insert(rows,"From: "..sender)
		table.insert(rows,'"'..self:get_snippet{assert(data.subject)}..'"')
		local time = assert(data.time_received)
		item.header = {assert(data.subject).." (%s)",time}
		local from = "From "..sender
		if sender ~= data.contact_email then
			from = from .." ("..data.contact_email..")"
		end
		table.insert(item.body, from)
		for i,line in ipairs(data.body_preview) do
			table.insert(item.body,line)
		end
	elseif command == "calendar_event" then
		typ = "calendar"
		table.insert(rows,"CALENDAR EVENT")
		table.insert(rows,self:get_snippet{assert(data.description)})
		local time = data.time or os.time()
		table.insert(rows,self:text_or_time{"When: %s",time})
		item.header = {data.description.." (%s)",time}
		item.time = time
		if data.location then
			table.insert(item.body,"Where: "..data.location)
		end
		if data.contact_name then
			table.insert(item.body,"With: "..data.contact_name)
		end
		if data.details then
			table.insert(item.body, "Details: "..data.details)
		end
	else
		error("Unknown from_phone command: "..tostring(command))
	end
	
	for i, row in ipairs(rows) do
		if type(row) == "string" then
			rows[i] = dynawa.bitmap.text_lines{text = row, width = 140, autoshrink = true, center = true}
		end
	end
	if icon then
		table.insert(rows,2,assert(dynawa.bitmap.from_png(icon)))
		item.icon = assert(data.contact_icon["30"])
	end
	local bmap = dynawa.bitmap.layout_vertical(rows, {align = "center", border = 5, spacing = 3, bgcolor={80,0,80}})
	dynawa.bitmap.border(bmap,1,{255,255,255})
	local prev_popup
	local win = dynawa.window_manager:peek()
	if win and win.app == dynawa.popup then
		prev_popup = win.id
	end
	local new_popup = dynawa.popup:open{bitmap = bmap, autoclose = 20000, on_confirm = function()
		log("Showing message")
		self:show_message{folder_id = typ, message = item}
	end}
	local folder = assert(self.prefs.storage[typ])
	table.insert(folder,1,item)
	if typ == "calendar" then --Calendar folder is sorted by event time
		table.sort(folder,function(a,b)
			return ((a.time or 0) > (b.time or 0))
		end)
	end
	local limit = 20 --Message cap per folder. #todo increase!
	while #folder > limit do
		table.remove(folder)
	end
	
	self:save_data(self.prefs)
	self:broadcast_update()
	if not prev_popup or (self.popup_window_id ~= prev_popup) then
		--Don't vibrate if my popup is already displayed.
		dynawa.devices.vibrator:alert()
	end
	self.popup_window_id = new_popup
end

function app:get_snippet(lines)
	local snippet = assert(lines[1])
	if #snippet <= 30 then
		return snippet
	end
	return snippet:sub(1,30).."..."
end

function app:text_or_time(arg)
	if type(arg)=="string" then
		return arg
	end
	local t_mins = math.floor((arg[2] - os.time() + 30) / 60)
	local result = "right now"
	if t_mins ~= 0 then
		result = {}
		local future = t_mins > 0
		t_mins = math.abs(t_mins)
		local t_weeks = math.floor(t_mins / 60 / 24 / 7)
		local t_days = math.floor(t_mins / 60 / 24) % 7
		local t_hours = math.floor(t_mins / 60) % 24
		t_mins = t_mins % 60
		if t_weeks == 1 then
			table.insert(result, "1 week")
		elseif t_weeks > 1 then
			table.insert(result, t_weeks.." weeks")
		end
		if t_days == 1 then
			table.insert(result, "1 day")
		elseif t_days > 1 then
			table.insert(result, t_days.." days")
		end
		if t_hours == 1 then
			table.insert(result, "1 hour")
		elseif t_hours > 1 then
			table.insert(result, t_hours.." hours")
		end
		if t_mins == 1 then
			table.insert(result, "1 minute")
		elseif t_mins > 1 then
			table.insert(result, t_mins.." minutes")
		end
		result = table.concat(result, ", ")
		if future then
			result = "in " .. result
		else
			result = result .. " ago"
		end
	end
	result = string.format(arg[1], result)
	return result
end

function app:count(typ)
	local unread = 0
	for i,item in ipairs(self.prefs.storage[typ]) do
		if not item.read then
			unread = unread + 1
		end
	end
	return unread, #self.prefs.storage[typ]
end

function app:count_str(typ)
	local unread, all = self:count(typ)
	return unread.."/"..all, (unread > 0)
end

function app:switching_to_front()
	self:display_root_menu()
end

function app:display_root_menu()
	local menu = {
		banner = "Inbox",
		flags = {root = true},
		items = {}
	}
	for i, id in ipairs(inbox_ids) do
		local item = {render = function(_self, args)
			local newstr, is_new = self:count_str(id)
			local color
			if is_new then
				color = highlight_color
			end
			local width = args.max_size.w
			local bitmap = dynawa.bitmap.new(width, 25,0,0,0,0)
			dynawa.bitmap.combine(bitmap, self.gfx[id],width - 25, 0)
			dynawa.bitmap.combine(bitmap, dynawa.bitmap.text_line(inbox_names[id]..":","/_sys/fonts/default10.png"), 0, 10)
			dynawa.bitmap.combine(bitmap, dynawa.bitmap.text_line(newstr, "/_sys/fonts/default15.png", color),87,6)
			return bitmap
		end}
		item.value = {open_folder = id}
		table.insert (menu.items, item)
	end
	table.insert(menu.items, {text = "Mark all as read", selected = function(_self,args)
		for i,id in ipairs(inbox_ids) do
			self:mark_all_read(id)
		end
		dynawa.popup:info("Contents of all folders marked as read.")
		args.menu:invalidate()
		self:save_data(self.prefs)
		self:broadcast_update()
	end})
	table.insert(menu.items, {text = "Delete all", selected = function(_self,args)
		for i,id in ipairs(inbox_ids) do
			self.prefs.storage[id] = {}
		end
		dynawa.popup:info("Contents of all folders deleted.")
		args.menu:invalidate()
		self:save_data(self.prefs)
		self:broadcast_update()
	end})
	local menuwin = self:new_menuwindow(menu)
	menuwin:push()
end

function app:menu_item_selected(args)
	local value = args.item.value
	if not value then
		return
	end
	if value.open_folder then
		self:display_folder(value.open_folder)
	elseif value.message then
		self:show_message{folder_id = args.menu.flags.folder_id, message = value.message}
	else
		error("WTF?")
	end
end

function app:display_folder(folder_id)
	local folder = self.prefs.storage[folder_id]
	local menu = {
		banner = "Inbox: "..assert(inbox_names[folder_id]),
		flags = {parent = assert(dynawa.window_manager:peek().menu), folder_id = folder_id},
		items = {},
	}

	for i, item in ipairs(folder) do
		local item = {value = {message = item}}
		item.render = function(_self,args)
			local color
			local message = _self.value.message
			if not message.read then
				color = highlight_color
			end
			local bitmap
			if message.icon then
				local icon = assert(dynawa.bitmap.from_png(message.icon),"Cannot parse icon from PNG")
				local iw,ih = dynawa.bitmap.info(icon)
				local width = args.max_size.w - iw - 1
				assert(width >= 50, "Item width (minus icon) is less than 50")
				local textbmp = dynawa.bitmap.text_lines{text = "> "..self:text_or_time(_self.value.message.header), color = color, width = width}
				bitmap = dynawa.bitmap.layout_horizontal({textbmp, icon},{align="top", spacing = 1})
				local w,h = dynawa.bitmap.info(bitmap) --#todo remove after testing!
				assert (w == args.max_size.w, "Mismatch "..w.." / "..args.max_size.w) --#todo remove after testing!
			else
				bitmap = dynawa.bitmap.text_lines{text="> "..self:text_or_time(_self.value.message.header), color = color, width = assert(args.max_size.w)}
			end
			return bitmap
		end
		table.insert (menu.items, item)
	end
	if not next(menu.items) then --No items in menu
		table.insert (menu.items,{text="[No messages in this folder]", textcolor = {255,255,0}})
	else

		table.insert(menu.items, {text = "Mark all as read", selected = function(_self,args)
			self:mark_all_read(folder_id)
			dynawa.popup:info("Contents of this folder marked as read.")
			args.menu:invalidate()
			args.menu.flags.parent:invalidate()
			self:save_data(self.prefs)
			self:broadcast_update()
		end})
		
		table.insert(menu.items, {text = "Delete all", selected = function(_self,args)
			self.prefs.storage[folder_id] = {}
			args.menu.flags.parent:invalidate()
			args.menu.window:pop()
			args.menu:_delete()
			dynawa.popup:info("Contents of this folder deleted.")
			self:save_data(self.prefs)
			self:broadcast_update()
		end})
	end

	local menuwin = self:new_menuwindow(menu)
	menuwin:push()
end

function app:broadcast_update() --Broadcast the change
	self.events:generate_event{type = "inbox_updated", folders = self.prefs.storage}
end

function app:mark_all_read(box_id)
	for i,item in ipairs(assert(self.prefs.storage[box_id])) do
		item.read = true
	end
end

--Show single message details
--The exact m.o. depends on whether this is called from folder menu or not
function app:show_message(args)
	dynawa.busy()
	local folder_id, message = assert(args.folder_id, "No folder"), assert(args.message, "No message")
	local topwin = dynawa.window_manager:peek()
	if not (topwin and topwin.app == self and topwin.menu and topwin.menu.flags and topwin.menu.flags.folder_id == folder_id) then
		--We are not being called from folder. Close all active apps, open inbox and relevant folder.
		dynawa.window_manager:stack_cleanup()
		self:display_root_menu()
		self:display_folder(folder_id)
		topwin = dynawa.window_manager:peek()
	end
	local menu = {flags = {parent = assert(topwin.menu)}, items = {}}
	menu.banner = self:text_or_time(message.header)
	for i, line in ipairs(message.body) do
		table.insert(menu.items, {text = self:text_or_time(line), textcolor = {255,255,0}})
	end
	dynawa.busy()
	if not message.read then
		message.read = true
		topwin.menu:invalidate()
		topwin.menu.flags.parent:invalidate()
		self:save_data(self.prefs)
		self:broadcast_update()
	end
	table.insert(menu.items, {text = "Delete this message", selected = function(_self,args)
		local folder_id = assert(args.menu.flags.parent.flags.folder_id)
		for i, msg_iter in ipairs(self.prefs.storage[folder_id]) do
			if msg_iter == message then
				table.remove(self.prefs.storage[folder_id],i)
				break
			end
		end
		dynawa.window_manager:pop():_delete() --Pop the message menu
		dynawa.window_manager:pop():_delete() --And the original folder menu
		self:display_folder(folder_id)
		dynawa.popup:info("Message deleted.")
		self:save_data(self.prefs)
		self:broadcast_update()
	end})
	dynawa.busy()
	local menuwin = self:new_menuwindow(menu)
	menuwin:push()
end
