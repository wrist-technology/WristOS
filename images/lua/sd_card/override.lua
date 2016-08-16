_G.dynawa.debug = {}

local serfunc0 = function (x) return (string.format("%q","<!"..tostring(x)..">")) end

local serfunc = {
	boolean = function (x) return tostring(x) end,
	number = function (x) return tostring(x) end,
	string = function(str) return string.format("%q",str) end,
	userdata = serfunc0,
	["function"] = serfunc0,
}

local function serialize(neco)
	if type(neco)=="table" then
		local result = {}
		for k,v in pairs(neco) do
			local serk="["..serialize(k).."]"
			table.insert(result, serk.."="..serialize(v))
		end
		table.sort(result)
		return "{"..table.concat(result,",").."}"
	else
		local fun = serfunc[type(neco)]
		if fun then
			return fun(neco)
		end
		return "?!?"..tostring(neco).."?!?"
	end
end

local function split_string(str)
	assert(type(str)=="string" and #str > 0)
	local chunks={}
	local chunksize=100
	for pointer = 1, #str, chunksize do
		local to = pointer + chunksize - 1
		if to > #str then
			to = #str
		end
		local chunk=str:sub(pointer,to)
		chunk = chunk:gsub(".",function(chr)
			if chr >"!" and chr <= "~" then
				return chr
			else
				return string.format("!%03d",string.byte(chr))
			end
		end)
		table.insert(chunks, chunk)
	end
	return chunks
end

function dynawa.debug.send_raw(str)
	io.write(str.."\n")
	io.output():flush()
end

function dynawa.debug.receive_raw()
	local line = io.read("*l")
	assert(line,"Cannot read from stdin")
	return line
end

local function chunks_decode(str)
	local decoded=str:gsub("!(%d%d%d)",function(ch4)
		return string.char(tonumber(ch4))
	end)
	return decoded
end

function dynawa.debug.receive(line1)
	local rec = dynawa.debug.receive_raw
	if not line1 then
		line1 = rec()
	end
	assert(line1=="DATA_START","Expected DATA_START, got "..line1.." instead")
	
	local chunks = {"return "}
	repeat
		local chunk = rec()
		if chunk == "DATA_END" then
			chunk=nil
		end
		if chunk then
			local str=assert(chunk:match("CHUNK (%S+)"),"No chunk data in "..chunk)
			table.insert(chunks,str)
		end
	until not chunk
	local data = chunks_decode(table.concat(chunks))
	data = loadstring(data)()
	assert(type(data)=="table","Received data is not table but "..type(data))
	return data
end

function dynawa.debug.send(data)
	assert(type(data)=="table", "Data is not a table but "..type(data))
	local ser = assert(serialize(data))
	local send_raw = dynawa.debug.send_raw
	local chunks = split_string(ser)
	send_raw("DATA_START")
	for i, chunk in ipairs(chunks) do
		send_raw("CHUNK "..chunk)
	end
	send_raw("DATA_END")
end

local errfunc=function(errtxt)
	local i = 1
	local obj
	repeat
		local name,value = debug.getlocal(2,i)
		if name == "self" then
			obj = value
			break
		end
		i = i + 1
	until not name
	local txt = errtxt
	if obj then
		txt = txt.."\nValue of 'self' = "..tostring(obj)
	end
	local tback = debug.traceback(txt,2)
	return tback
end
	
function dynawa.debug.update_file() --Receive one file and store it to SD card
	local data = dynawa.debug.receive(reply)
	local fname = assert(data.filename)
	local file = assert(data.file)
	--create directory
	local parts = {}
	fname:gsub("(.-/)", function(part)
		table.insert(parts,part)
	end)
	assert (parts[1]=="/")
	if #parts > 1 then
		parts[2] = parts[1]..parts[2]
		table.remove(parts,1)
		local dirname = ""
		for i = 1, #parts do
			dirname = dirname .. parts[i]
			local dirname2 = dirname:match("(.*)/$")
			--log("Creating dir: "..dirname)
			dynawa.file.mkdir(dirname2)
		end 
	end
	
	--dynawa.debug.send_raw("WRITING_FILE "..fname)
	local fd = assert(io.open(fname,"w"),"Cannot open file "..fname.." for writing")
	fd:write(file)
	fd:close()
	--dynawa.debug.send_raw("FILE_UPDATED")
end

function dynawa.debug.whats_new()
	dynawa.debug.send_raw("WHATS_NEW?")
	local flip_flop
	local indicator = function(r,g,b)
		dynawa.bitmap.show_partial(dynawa.bitmap.new(50,50,r,g,b),nil,nil,nil,nil,0,0)
	end
	while true do
		flip_flop = not flip_flop
		if flip_flop then
			indicator(50,0,50)
		else
			indicator(100,255,100)
		end
		
		local msg=dynawa.debug.receive_raw()
		if msg == "RESTART" then
			dynawa.debug.send_raw("RESTARTING")
			if dynawa.bt then
				dynawa.bt.cmd(2) --BT off
			end
			boot_init()
			return
		elseif msg == "FILE" then
			dynawa.debug.update_file()
		elseif msg == "BYE" then
			return
		else
			--log ("Ignoring unknown msg from PC: "..msg)
		end
	end
end

function dynawa.debug.main_handler(msg)
	--dynawa.debug.send_raw("DEBUG.MAIN_HANDLER")
	--log(msg.button..":"..msg.type)
	if msg.type == "button_hold" and msg.button == 1 then --update files and restart WristOS]
		dynawa.debug.whats_new()
		return
	end
	
	local protected=function()
		if not dynawa.version then
			dofile(dynawa.dir.sys.."wristos.lua")
		end
		return _G.private_main_handler(msg)
	end

	local status,result=xpcall(protected,errfunc)
	if not status then --runtime error caught
		dynawa.debug.send{runtime_error=result}
	end
end

_G.log = function(stuff)
	dynawa.debug.send_raw("LOG: "..tostring(stuff))
end

