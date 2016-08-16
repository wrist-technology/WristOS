--File functions

--Strictly speaking, the serialize() function should throw an error when presented with userdata or function.
--However, we silently accept them in the case someone is using it for storing raw (debugging) structures.
local serfunc0 = function (x) return (string.format("%q","<!"..tostring(x)..">")) end

local serfunc = {
	boolean = function (x) return tostring(x) end,
	number = function (x) return tostring(x) end,
	string = function(str) return string.format("%q",str) end,
	userdata = serfunc0,
	["function"] = serfunc0,
}

dynawa.file.serialize = function (neco)
	if type(neco)=="table" then
		local result = {}
		for k,v in pairs(neco) do
			local serk="["..dynawa.file.serialize(k).."]"
			table.insert(result, serk.."="..dynawa.file.serialize(v))
		end
		--table.sort(result)
		return "{"..table.concat(result,", ").."}"
	else
		local fun = serfunc[type(neco)]
		if fun then
			return fun(neco)
		end
		error("Cannot serialize data of type '"..type(neco).."'")
	end
end

dynawa.file.load_data = function(fname)
	local fd, err = io.open(fname,"r")
	if not fd then
		return nil, err
	end
	local txt = assert(fd:read("*a"))
	fd:close()
	local chunk = assert(loadstring("return "..txt))
	local data = chunk()
	assert(type(data) == "table", "Preferences file evaluated not to table but to "..tostring(data))
	return data
end

dynawa.file.save_data = function(data, fname)
	assert(type(data) == "table", "Data is not a table but "..tostring(data))
	local txt = dynawa.file.serialize(data)
	local fd = assert(io.open(fname,"w"))
	fd:write(txt)
	fd:close()
	return true
end

dynawa.file.save_settings = function(x)
	assert(not x,"Dynawa.file.save_settings accepts NO arguments")
	return dynawa.file.save_data(dynawa.settings, dynawa.dir.sys.."settings.data")
end
