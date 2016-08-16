--menus for SuperMan

my.globals.menus.root = function()
	local menu = {banner = "SuperMan root"}
	menu.items = {
		{text = "Shortcuts", after_select = {go_to="shortcuts:"}},
		{text = "Apps", after_select = {go_to = "apps"}},
		{text = "File Browser", after_select = {go_to="file_browser"}},
		{text = "Adjust time and date", after_select = {go_to = "adjust_time_date"}},
		{text = "Default font size", after_select = {go_to = "default_font_size"}},
	}
	return menu
end

my.globals.menus.shortcuts = function(arg)
	local scuts = assert(dynawa.settings.superman.shortcuts)
	local menu = {banner = "SuperMan shortcuts", items={}, always_refresh = true}
	if not next(scuts) then
		table.insert(menu.items,{text = 'No shortcuts defined. Add them using "Add shortcut" command in other menus.'})
		return menu
	end
	for k,v in pairs(scuts) do
		table.insert(menu.items,{text=v.text, timestamp = v.timestamp, after_select = {go_to = k}})
	end
	table.sort(menu.items, function(a,b)
		return (a.timestamp > b.timestamp)
	end)
	return menu
end

local function app_menu2(message)
	if message.reply then
		local menu = message.reply
		menu.proxy = assert(message.sender.app)
		dynawa.message.send{type = "open_my_menu", menu = menu}
	else
		dynawa.message.send{type="open_popup", text="This app has no menu", style = "error"}
	end
end

my.globals.menus.app_menu = function(app_id)
	local app = assert(dynawa.apps[app_id],"There is no active app with id '"..app_id.."'")
	dynawa.message.send{type = "your_menu", receiver = app, reply_callback = app_menu2}
end

my.globals.menus.file_browser = function(dir)
	if not dir then
		dir = "/"
	end
	--log("opening dir "..dir)
	local dirstat = dynawa.file.dir_stat(dir)
	local menu = {banner = "File browser: "..dir, items={}, always_refresh = true, allow_shortcut = "Dir: "..dir}
	if not dirstat then
		table.insert(menu.items,{text="[Invalid directory]"})
	else
		if next(dirstat) then
			if dirstat["main.lua"] then --This dir is an app
				if dynawa.apps[dir] then
					table.insert(menu.items,{text = "+ See details of this running app", sort = "00", after_select = {go_to = "app:"..dir}})
				else
					table.insert(menu.items,{text = "+ Start this app", sort = "00", after_select={popup = "App started.", refresh_menu = true}, callback = function()
						dynawa.app.start(dir)
					end})
				end
			end
			for k,v in pairs(dirstat) do
				local txt = k.." ["..v.." bytes]"
				local sort = "2"..txt
				if v == "dir" then 
					txt = "= "..k
					sort = "1"..txt
				end
				--log("Adding dirstat item: "..txt)
				local location = "file:"..dir..k
				if v == "dir" then
					location = "file_browser:"..dir..k.."/"
				end
				table.insert(menu.items,{text = txt, sort = sort, after_select={go_to = location}})
			end
			table.sort(menu.items,function(it1,it2)
				return it1.sort < it2.sort
			end)
		else
			table.insert(menu.items,{text="[Empty directory]"})
		end
	end
	return menu
end

my.globals.menus.file = function(fullname)
	assert(fullname)
	assert(#fullname > 0)
	local dir, fname = fullname:match("(.*/)(.*)")
	assert(fname)
	--log("DIR:"..dir)
	--log("FNAME:"..fname)
	local size = assert(dynawa.file.dir_stat(dir))[fname]
	assert(type(size) == "number")
	local menu = {banner = "File: "..fname, allow_shortcut = "File: "..fullname, items = {}}
	table.insert(menu.items,{text="Size: "..size.." bytes"})
	if fullname:match("%.data$") then
		table.insert(menu.items,{text = "Browse data hierarchy", after_select = {go_to = "data_browser"}, callback = function()
			local data = assert(dynawa.file.load_data(fullname))
			my.globals.data_browser={data=data, location = "Data browser: "..fname}
		end})
	end
	table.insert(menu.items,{text="File operations #TBD"})
	my.globals.data_browser = nil
	return menu
end

local function dump_string(value)
	if type(value)~="string" then
		return tostring(value)
	end
	if #value > 100 then
		value = value:sub(1,100)
	end
	local binary = false
	for i=1, #value do
		local char = value:sub(i,i)
		if char < " " or char > "~" then
			binary = true
			break
		end
	end
	if binary then
		local bytes = {}
		for i=1, #value do
			table.insert(bytes, string.format("%02x",string.byte(value:sub(i,i))))
		end
		value = table.concat(bytes, " ")
	else
		value = '"'..value..'"'
	end
	return value
end

my.globals.menus.data_browser = function()
	local data = assert(my.globals.data_browser.data, "Data browser invoked but no data stored")
	local menu = {banner = my.globals.data_browser.location, items = {}}
	assert(type(data) == "table", "Data is not a table")
	if not next(data) then
		table.insert(menu.items,{text = "[Empty]"})
	else
		local keys = {}
		for k,v in pairs(data) do
			table.insert(keys, k)
		end
		table.sort(keys, function(a,b)
			if type(a) ~= type(b) then
				return (type(a) < type(b))
			end
			return (a < b)
		end)
		for i, key in ipairs(keys) do
			local value = data[key]
			if type(value) == "table" then
				table.insert(menu.items,{text = "= "..tostring(key), after_select = {go_to = "data_browser"}, callback = function()
					my.globals.data_browser = {data = value, location = my.globals.data_browser.location .. "/"..key}
				end})
			else
				table.insert(menu.items,{text = dump_string(key).." : "..dump_string(value)})
			end
		end
	end
	return menu
end

my.globals.menus.default_font_size = function(dir)
	local dir = dynawa.dir.sys.."fonts/"
	local dirstat = assert(dynawa.file.dir_stat(dir),"Cannot open fonts directory")
	local menu = {banner = "Select default system font:", items = {}}
	for k,v in pairs(dirstat) do
		if type(v) == "number" then
			local font_id = dir..k
			local bitmap,w,h = dynawa.bitmap.text_lines{text = "Quick brown fox jumped over the lazy dog", font = font_id}
			table.insert(menu.items,{bitmap = bitmap, fontsize = h, value = {result = "default_font_changed", font_id = font_id}, 
				after_select = {close_menu = true, popup = "Default system font changed"}})
		end
	end
	table.sort(menu.items, function(a,b)
		return (a.fontsize < b.fontsize)
	end)
	return menu
end

my.globals.results.default_font_changed = function(value)
	local font_id = assert(value.font_id)
	dynawa.settings.default_font = font_id
	dynawa.file.save_settings()
end

my.globals.menus.adjust_time_date = function(what)
	local date = assert(os.date("*t"))
	--log("what = "..tostring(what))
	if not what then
		local menu = {banner = "Adjust time & date", always_refresh = true}
		menu.items = {
			{text = "Day of month: "..date.day, after_select = {go_to="adjust_time_date:day"}},
			{text = "Month: "..date.month, after_select = {go_to="adjust_time_date:month"}},
			{text = "Year: "..date.year, after_select = {go_to="adjust_time_date:year"}},			
			{text = "Hours: "..date.hour, after_select = {go_to="adjust_time_date:hour"}},
			{text = "Minutes: "..date.min, after_select = {go_to="adjust_time_date:min"}},			
		}
		return menu
	end
	local limit = {from=2001, to=2060, name = "year"} --year
	if what=="month" then
		limit = {from = 1, to = 12, name = "month"}
	elseif what=="day" then
		limit = {from = 1, to = 31, name = "day of month"}
	elseif what == "hour" then
		limit = {from = 0, to = 23, name = "hours"}
	elseif what == "min" then
		limit = {from = 0, to = 59, name = "minutes"}
	end
	local menu = {banner = "Please adjust the "..limit.name.." value", items = {}}
	for i = limit.from, limit.to do
		local popup = "Value adjusted"
		if what == "min" then
			popup = "Value adjusted and seconds set to 00"
		end
		local item = {text = tostring(i), value = {what = what, number = i, result = "adjusted_time_date"}, 
			after_select = {popup = popup, go_back = true}}
		table.insert(menu.items,item)
		if i == date[what] then
			menu.active_value = item.value
		end
	end
	return menu
end

my.globals.results.adjusted_time_date = function(message)
	local date = assert(os.date("*t"))
	date[message.what] = message.number
	if message.what == "min" then
		date.sec = 0
	end
	local secs = assert(os.time(date))
	dynawa.time.set(secs)
end


