--Various Apps menus for SuperMan

local function system_app(app_id)
	assert(type(app_id) == "string")
	--log("matching "..app_id.." with ^"..dynawa.dir.sys)
	return (app_id:match("^"..dynawa.dir.sys))
end

my.globals.menus.apps = function(dir)
	local menu = {banner = "Apps", allow_shortcut = true}
	menu.items = {
		{text = "Running apps", after_select = {go_to="apps_running:"}},
		{text = "Stopped apps", after_select = {go_to="apps_stopped:"}},
		{text = "Configure app directories"},
	}
	return menu
end

local function overview_received(message)
	if message.reply then
		local bitmap = message.reply
		assert(dynawa.bitmap.info(bitmap))
		local app = assert(message.original_message.receiver)
		my.globals.overviews.bitmaps[app] = bitmap
	end
	local apps = assert(my.globals.overviews.apps)
	apps[app] = nil
	if next(apps) then 
		return -- Someone still has not responded
	end
	local over = my.globals.overviews
	my.globals.overviews = nil
	callback{menu = over.menu, bitmaps = over.bitmaps}
end

local function get_overviews(args)
	local apps0 = assert(args.apps)
	local callback = assert(args.callback)
	local menu = assert(args.menu)
	local apps = {}
	my.globals.overviews = {apps = apps, bitmaps = {}, callback = callback, menu = menu}
	for i, app in ipairs(apps0) do
		apps[app] = true
		dynawa.message.send{type="your_overview", receiver = app, reply_callback = overview_received}
	end
end

my.globals.menus.apps_running = function(dir)
	local menu = {banner = "Running apps", items = {}, allow_shortcut = true, always_refresh = true, hooks = {}}
	local apps = {}
	for key, val in pairs(dynawa.apps) do
		table.insert(apps, val)
		--log("app:"..val.id)
	end
	table.sort(apps, function (a,b)
		return (a.name < b.name)
	end)
	for i, app in ipairs(apps) do
		local item = {text = app.name, after_select = {go_to="app:"..app.id}}
		table.insert(menu.items,item)
		menu.hooks[app] = true
		dynawa.message.send{type = "your_overview", receiver = app, reply_callback = function(msg)
			if msg.reply then
				local bitmap = msg.reply
				item.text = nil
				item.bitmap = bitmap
			end
			dynawa.message.send{type = "superman_hook_done", hook = app, menu = menu}
		end}
	end
	return menu
end

local function dive_into_dir(dir, result)
	dynawa.busy()
	local dirstat = assert(dynawa.file.dir_stat(dir))
	if dirstat["main.lua"] then
		if not dynawa.apps[dir] then
			table.insert(result,dir)
		end
		return result
	end
	for k,v in pairs(dirstat) do
		if v=="dir" then
			dive_into_dir(dir..k.."/", result)
		end
	end
	return result
end

my.globals.menus.apps_stopped = function(dir)
	local menu = {banner = "Stopped apps", items = {}, allow_shortcut = true, always_refresh = true}
	local apps = dive_into_dir(dynawa.dir.apps,{})
	table.sort(apps, function (a,b)
		return (a < b)
	end)
	for i, app in ipairs(apps) do
		local item = {text = app, after_select = {go_to="file_browser:"..app}}
		table.insert(menu.items,item)
	end
	return menu
end

my.globals.menus.app = function (app_id)
	assert(app_id)
	local app = dynawa.apps[app_id]
	if not app then
		return my.globals.menus.file_browser(app_id)
	end
	local menu = {banner = "App: "..app.name, allow_shortcut = true, always_refresh = true, items = {}}
	local function additem (item)
		table.insert(menu.items, item)
	end
	additem{text = "Directory: "..app.id, after_select = {go_to="file_browser:"..app.id}}
	if app.priority then
		additem {text = "Priority: "..app.priority}
	end
	if system_app(app.id) then
		additem {text = "System app (unstoppable)"}
	else
		local auto = dynawa.settings.autostart[app.id]
		local appid = app.id
		if auto then
			additem {text = "Disable autostart", after_select = {refresh_menu = true, popup = "Autostart disabled. Restart the watch to stop the app."}, callback = function()
				dynawa.settings.autostart[appid] = nil
				dynawa.file.save_settings()
			end}
		else
			additem {text = "Enable autostart", after_select = {refresh_menu = true, popup = "Autostart enabled for this app."}, callback = function()
				dynawa.settings.autostart[appid] = {} --#todo priority etc
				dynawa.file.save_settings()
			end}
		end
	end
	additem {text = "Try opening its menu", after_select = {go_to="app_menu:"..app.id}}
	menu.hooks = {bitmap = true}
	dynawa.message.send{type = "your_overview", receiver = app, reply_callback = function(msg)
		if msg.reply then
			local item = {bitmap = msg.reply}
			table.insert(menu.items, 1, item)
		end
		dynawa.message.send{type = "superman_hook_done", hook = "bitmap", menu = menu}
	end}
	return menu
end

