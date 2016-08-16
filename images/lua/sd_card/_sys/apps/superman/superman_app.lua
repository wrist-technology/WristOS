app.name = "SuperMan"
app.id = "dynawa.superman"

function app:open_menu_by_url(url)
	local url0, urlarg = url:match("(.+):(.*)")
	if not urlarg then
		url0 = url
	end
	if urlarg == "" then
		urlarg = nil
	end
	local builder = self.menu_builders[url0]
	if not builder then
		error("Cannot get builder for url: "..url)
	end
	local menu = assert(builder(self,urlarg),"Builder didn't return menu nor menu descriptor")
	if not menu.is_menu then
		menu = self:new_menuwindow(menu).menu
	end
	menu.outer_color = {255,0,0}
	menu.url = url
	self:open_menu(menu)
	return menu
end

function app:open_menu(menu)
	assert(menu.window)
	self:push_window(menu.window)
	return menu
end

function app:menu_cancelled(menu)
	local popwin = dynawa.window_manager:pop()
	assert (popwin == menu.window)
	popwin:_delete()
	local window = dynawa.window_manager:peek()
	if window then
		if window.menu:requires_render() then
			log(window.menu .. "requires re-render")
		else
			log(window.menu .. "does not require re-render")
		end
	else
		dynawa.window_manager:show_default()
	end
end

function app:switching_to_front()
	self:open_menu_by_url("root")
end

function app:switching_to_back()
	dynawa.window_manager:pop_and_delete_menuwindows()
end

function app:menu_item_selected(args)
	local menu = args.menu
	assert (menu.window.app == self)
--	log("selected item "..args.item)
	local value = args.item.value
	if not value then
		return
	end
	if value.go_to_url then
		local newmenu = self:open_menu_by_url(value.go_to_url)
		return
	end
end

function app:start()
	dynawa.superman = self
end

app.menu_builders = {}

function app.menu_builders:root()
	local menu_def = {
		banner = {
			text="SuperMan root menu"
			},
		items = {
--			{text = "Shortcuts", value = {go_to_url = "shortcuts"}},
			{text = "Apps", value = {go_to_url = "apps"}},
			{text = "File browser", value = {go_to_url = "file_browser"}},
			{text = "Display settings", value = {go_to_url = "adjust_display"}},
			{text = "Time and date settings", value = {go_to_url = "adjust_time_date"}},
			{text = "Gestures (accelerometer)", value = {go_to_url = "adjust_gestures"}},
			{text = "Debug menu", value = {go_to_url = "debug"}},
		},
	}
	return menu_def
end

function app.menu_builders:adjust_display()
	local menudesc = {
		banner = "Adjust display", items = {
			{text = "Default font size", value = {go_to_url = "default_font_size"}},
			{text = "Display brightness", value = {go_to_url = "display_brightness"}},
			{text = "Display autosleep", selected =
				function()
					local sandman = assert(dynawa.app_manager:app_by_id("dynawa.sandman"))
					sandman:switching_to_front()
				end
			},
		}
	}
	local menu = self:new_menuwindow(menudesc).menu
	return menu
end

function app.menu_builders:display_brightness()
	local choices = {[0] = "Auto brightness", "Min. brightness", "Normal brightness", "Max. brightness"}
	local menudesc = {banner = "Display brightness: "..choices[dynawa.settings.display.brightness], items = {}}
	for i = 0,3 do
		table.insert(menudesc.items,{
			text = choices[i],
			selected = function()
				dynawa.devices.display.brightness(i)
				dynawa.settings.display.brightness = i
				dynawa.file.save_settings()
			end
		})
	end
	return menudesc
end

function app.menu_builders:default_font_size()
	local menudesc = {banner = "Select default font size:", items = {}}
	local font_sizes = {7,10,15}
	for i, size in ipairs(font_sizes) do
		local item = {text = "Quick brown fox jumps over the lazy dog ("..size.." px)",
				value = {font_size = size, font_name = "/_sys/fonts/default"..size..".png"}}
		item.render = function(_self,args)
			return dynawa.bitmap.text_lines{text = _self.text, font = assert(_self.value.font_name), width = assert(args.max_size.w)}
		end
		table.insert(menudesc.items, item)
	end
	menudesc.item_selected = function(self,args)
		dynawa.settings.default_font = args.item.value.font_name
		dynawa.file.save_settings()
		dynawa.popup:info("Default font changed to size "..args.item.value.font_size)
	end
	return menudesc
end

function app.menu_builders:file_browser(dir)
	if not dir then
		dir = "/"
	end
	--log("opening dir "..dir)
	local dirstat = dynawa.file.dir_stat(dir)
	local menu = {banner = "File browser: "..dir, items={}}
	if not dirstat then
		table.insert(menu.items,{text="[Invalid directory]"})
	else
		if next(dirstat) then
			for k,v in pairs(dirstat) do
				local txt = k.." ["..v.." bytes]"
				local sort = "2"..txt
				if v == "dir" then 
					txt = "= "..k
					sort = "1"..txt
				end
				--log("Adding dirstat item: "..txt)
				local location = "file:"..dir..k
				local item = {text = txt, sort = sort}
				if v == "dir" then
					item.value = {go_to_url = "file_browser:"..dir..k.."/"}
				elseif k:match("%.wav$") then
					item.selected = function()
						local fname = dir..k
						log("Attempting to play sample: "..fname)
						dynawa.busy()
						local sample = dynawa.audio.sample_from_wav_file(fname, nil, nil, nil)
						if sample then
						    log("sample ok")
						    dynawa.audio.play(sample, nil, nil)
						else
						    log("sample nok")
						end
					end
				end
				table.insert(menu.items,item)
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

local adjust_time_selected = function(self,args)
	local menu = args.menu
	assert(menu == self)
	local value = assert(args.item.value)

	local date = assert(os.date("*t"))
	date[value.what] = value.number
	if value.what == "min" then
		date.sec = 0
	end
	local secs = assert(os.time(date))
	dynawa.time.set(secs)
	
	menu.window:pop()
	menu:_delete()
	local win = dynawa.window_manager:pop()
	win:_delete()
	dynawa.superman:open_menu_by_url("adjust_time_date")
	local msg = "Adjusted "..value.name
	if value.what == "min" then
		msg = msg.." and set seconds to zero."
	else
		msg = msg.."."
	end
	dynawa.popup:open{text = msg}
end

function app.menu_builders:adjust_time_date(what)
	local date = assert(os.date("*t"))
	--log("what = "..tostring(what))
	if not what then
		local menu = {banner = "Adjust time & date"}
		menu.items = {
			{text = "Day of month: "..date.day, value = {go_to_url="adjust_time_date:day"}},
			{text = "Month: "..date.month, value = {go_to_url="adjust_time_date:month"}},
			{text = "Year: "..date.year, value = {go_to_url="adjust_time_date:year"}},			
			{text = "Hours: "..date.hour, value = {go_to_url="adjust_time_date:hour"}},
			{text = "Minutes: "..date.min, value = {go_to_url="adjust_time_date:min"}},			
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
	local menu = {banner = "Please adjust the "..limit.name.." value", items = {}, item_selected = adjust_time_selected}
	for i = limit.from, limit.to do
		local item = {text = tostring(i), value = {what = what, number = i, name = limit.name}}
		table.insert(menu.items,item)
		if i == date[what] then
			menu.active_value = item.value
		end
	end
	return menu
end

function app.menu_builders:apps()
	local menudesc = {
		banner = "Apps", items = {
			{text = "Running Apps", value = {go_to_url = "apps_running"}},
			{text = "Non-running Apps on SD card", value = {go_to_url = "apps_on_card"}},
			{text = "Autostarting Apps", value = {go_to_url = "apps_autostart"}},
			{text = "Reorder switchable Apps (cycled using SWITCH button)", value = {go_to_url = "apps_switchable"}},
		}
	}
	return menudesc
end

function app.menu_builders:apps_running()
	local menudesc = {banner = "Running Apps", items = {}}
	local apps = {}
	for id, app in pairs(dynawa.app_manager.all_apps) do
		local name = "> "..id
		if app.name then
			name = name.." ("..app.name..")"
		end
		table.insert(menudesc.items, {text = name, value = {go_to_url = "app:"..id}})
	end
	table.sort(menudesc.items, function (a,b)
		return (a.text < b.text)
	end)
	return menudesc
end

function app.menu_builders:apps_on_card()
	local menudesc = {banner = "Apps on SD card (select to run)", items = {}}
	local apps = dynawa.app_manager:sd_card_apps()
	if not next(apps) then
		menudesc.items = {
			{text = "No non-running Apps found on SD card"}
		}
		return menudesc
	end
	table.sort(apps)
	for i,fname in ipairs(apps) do
		local nicefname = fname:gsub("/"," /")
		table.insert(menudesc.items,{text=">"..nicefname, value = {filename = fname}})
	end
	menudesc.item_selected = function(_self,args)
		local fname = assert(args.item.value.filename)
		local app,app2 = dynawa.app_manager:start_app(fname)
		if not app then
			app = assert(app2)
		end
		self:open_menu_by_url("app:"..app.id)
	end
	return menudesc
end

function app.menu_builders:apps_autostart()
	local menudesc = {banner = "Auto-starting apps", items = {}}
	for i,fname in ipairs(dynawa.app_manager:all_autostarting_apps()) do
		local app = dynawa.app_manager:app_by_filename(fname)
		if app then
			local name = app.name
			if name ~= app.id then
				name = name .. " ("..app.id..")"
			end
			if dynawa.app_manager:is_required(app) then
				name = "(REQUIRED) ".. name
			end
			local item = {text = "> "..name, value = {go_to_url = "app:"..app.id}}
			table.insert(menudesc.items,item)
		else
			log("App "..fname.." is listed as autostarting but is not running")
		end
	end
	if not next(menudesc.items) then
		table.insert(menudesc.items,{text="No autostarting Apps"})
	end
	return menudesc
end

function app.menu_builders:app(id)
	assert(id,"No App id provided")
	local app = dynawa.app_manager:app_by_id(id)
	if not app then
		local menudesc = {banner = "App with id: "..id, items = {}}
		table.insert(menudesc.items, {"This App is not running"})
		return menudesc
	end
	local menudesc = {banner = "App: "..app.name, items = {}}
	if dynawa.app_manager:can_be_switchable(app) then
		table.insert(menudesc.items,{text = "Switch to this App", selected = function(_self,args)
			log("Switching to front")
			app:switching_to_front()
		end})
	else
		table.insert(menudesc.items,{text = "Cannot switch to this App (provides no graphical output)"})
	end
	table.insert(menudesc.items,{text = "Id: "..app.id})
	local nicefname = app.filename:gsub("/"," /")
	table.insert(menudesc.items,{text = "Filename:"..nicefname})	
	local is_autostarting = dynawa.app_manager:is_autostarting(app)
	local auto = "disabled"
	if dynawa.app_manager:is_autostarting(app) then
		auto = "enabled"
	end
	if dynawa.app_manager:is_required(app) then
		table.insert(menudesc.items,{text = "Autostart: enabled (required App)"})
	else
		table.insert(menudesc.items,{text = "Autostart: "..auto, selected = function(_self,args)
			local menu = assert(args.menu)
			menu.window:pop():_delete()
			if auto == "disabled" then --set to autostart
				dynawa.app_manager:enable_autostart(app)
			else -- cancel autostart
				dynawa.app_manager:disable_autostart(app)
			end
			self:open_menu_by_url("app:"..app.id)
		end})
	end
	if dynawa.app_manager:can_be_switchable(app) then
		local switchable = "no"
		if dynawa.app_manager:is_switchable(app) then
			switchable = "yes"
		end
		table.insert(menudesc.items,{text = "Switchable: "..switchable, selected = function(_self,args)
			local menu = assert(args.menu)
			menu.window:pop():_delete()
			if switchable == "yes" then
				for i,id in ipairs(dynawa.settings.switchable) do
					if id == app.id then
						table.remove(dynawa.settings.switchable,i)
						break
					end
				end
			else
				table.insert(dynawa.settings.switchable,app.id)
				if not dynawa.app_manager:is_autostarting(app) then
					dynawa.app_manager:enable_autostart(app)
				end
			end
			self:open_menu_by_url("app:"..app.id)
			dynawa.file.save_settings()
		end})
	end
	return menudesc
end

function app.menu_builders:apps_switchable(index)
	local menudesc = {banner = "Switchable Apps", items = {}}
	for i,app in ipairs(dynawa.app_manager:all_switchable_apps()) do
		local name = app.name
		if app.name ~= app.id then
			name = name.." ("..app.id..")"
		end
		table.insert(menudesc.items,{text=name, value={go_to_url="apps_switchable_item:"..app.id}})
	end
	if not next(menudesc.items) then
		table.insert(menudesc.items,{text="No switchable Apps"})
	end
	if index then
		menudesc.active_item_index = assert(tonumber(index))
	end
	return menudesc
end

function app.menu_builders:apps_switchable_item(id)
	assert(id)
	local app = dynawa.app_manager:app_by_id(id)
	local menudesc = {banner = "Switchable App: "..app.name, items = {}}
	if not dynawa.app_manager:is_switchable(app) then
		--Safeguard - something has changed before the user clicked in the previous menu.
		table.insert(menudesc.items,{text = "This App is no longer switchable"})
		return menudesc
	end
	local switchable = dynawa.settings.switchable
	local index
	for i,id0 in ipairs(switchable) do
		if id0 == id then
			index = i
			break
		end
	end
	assert(index, "App id is not present in switchables table")
	local function back_to_switchables(newindex)
			if switchable[index] ~= id then
				dynawa.popup:error("App is no longer switchable")
				return
			end
			table.remove(switchable,index)
			table.insert(switchable,newindex,id)
			dynawa.file.save_settings()
			dynawa.window_manager:pop():_delete()
			dynawa.window_manager:pop():_delete()
			self:open_menu_by_url("apps_switchable:"..newindex)
	end
	if index > 1 then
		table.insert(menudesc.items,{text = "Move to top", selected = function (_self,args)
			back_to_switchables(1)
		end})
		table.insert(menudesc.items,{text = "Move one slot up", selected = function (_self,args)
			back_to_switchables(index - 1)
		end})
	end
	table.insert(menudesc.items,{text="Show App details", value={go_to_url="app:"..id}})
	if index < #switchable then
		table.insert(menudesc.items,{text = "Move one slot down", selected = function (_self,args)
			back_to_switchables(index + 1)
		end})
		table.insert(menudesc.items,{text = "Move to bottom", selected = function (_self,args)
			back_to_switchables(#switchable)
		end})
	end
	return menudesc
end

function app.menu_builders:adjust_gestures(index)
	local status = "OFF"
	if dynawa.settings.gestures.enabled then
		status = "ON"
	end
	local menudesc = {banner = "Gestures are "..status, items = {}}
	if status == "ON" then
		table.insert(menudesc.items,{text = "Turn gestures off", selected = function ()
			dynawa.settings.gestures.enabled = false
			dynawa.file.save_settings()
			dynawa.window_manager:pop():_delete()
			dynawa.popup:info("Gestures disabled")
		end})
	else
		table.insert(menudesc.items,{text = "Turn gestures on", selected = function ()
			dynawa.settings.gestures.enabled = true
			dynawa.file.save_settings()
			dynawa.window_manager:pop():_delete()
			dynawa.popup:info("Gestures enabled")
		end})
	end
	return menudesc
end

function app.menu_builders:debug(index)
	local menudesc = {banner = "Debug menu", items = {}}
	table.insert(menudesc.items,{text = "Turn SD card power management off", selected = function()
		dynawa.x.pm_sd(false)
		dynawa.popup:info("Sd card PM is now off. Reboot to turn it back on.")
	end})
	return menudesc
end
