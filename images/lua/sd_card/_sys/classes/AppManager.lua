local class = Class("AppManager")

function class:_init()
	self.all_apps = {}
	self.waiting_for = {}
	--The following apps are REQUIRED for WristOS to work
	self.required_apps = {
		"/_sys/apps/window_manager/window_manager_app.lua",
		"/_sys/apps/superman/superman_app.lua",
		"/_sys/apps/popup/popup_app.lua",
		"/_sys/apps/sandman/sandman_app.lua",
		"/_sys/apps/bluetooth_manager/bt_manager_app.lua",
	}
end

--Start app with provided filename. Returns the app object or false if app was already running.
function class:start_app(filename)
	--dynawa.busy()
	local dir = filename:match("(.*/).*%.lua$")
	if not dir then
		error("Cannot extract directory name from App filename: "..filename)
	end
	for id,app in pairs(self.all_apps) do
		if app.filename == filename then
			return false, app
		end
	end
	local chunk = assert(loadfile(filename))
	local app
	if filename:match("_bt_app%.lua$") then
		app = Class.BluetoothApp(filename)
	else
		app = Class.App(filename)
	end
	app.dir = dir
	app.filename = filename
	rawset(_G, "app", app)
	chunk()
	rawset(_G, "app", nil)
	assert(not self.all_apps[app.id], "App with id "..app.id.." is already running") --Should never get here!
	self.all_apps[app.id] = app
	app:start(app)
	if self.waiting_for[app.id] then
		for i,func in ipairs(self.waiting_for[app.id]) do
			func(app)
		end
		self.waiting_for[app.id] = nil
	end
	return app
end

function class:app_by_id(id)
	return self.all_apps[id]
end

function class:app_by_filename(fname)
	for id,app in pairs(self.all_apps) do
		if app.filename == fname then
			return app
		end
	end
	return nil
end

--Executes func only after the app "id" has been started
function class:after_app_start(id,func)
	assert(type(id)=="string")
	local app = self:app_by_id(id)
	if app then
		return func(app)
	end
	if not self.waiting_for[id] then
		self.waiting_for[id] = {}
	end
	table.insert(self.waiting_for[id],func)
end

--Returns FILENAMES(!!) of all autostarting apps ("required" + user)
function class:all_autostarting_apps()
	local apps = {}
	for i,id in ipairs(self.required_apps) do
		table.insert(apps, id)
	end
	for i,id in ipairs(dynawa.settings.autostart) do
		table.insert(apps, id)
	end
	return apps
end

--Returns all switchable Apps (actual apps, not IDs)
function class:all_switchable_apps()
	local result = {}
	for i, id in ipairs(dynawa.settings.switchable) do
		table.insert(result, assert(self:app_by_id(id)))
	end
	return result
end

function class:is_autostarting(app0)
	assert(app0.is_app)
	for i,id in ipairs(self:all_autostarting_apps()) do
		if app0.filename == id then
			return true
		end
	end
	return false
end

function class:start_everything()
	local apps = self:all_autostarting_apps()
	
	for i, app in ipairs(apps) do
		dynawa.busy(i / 2 / #apps + 0.5)
		self:start_app(app)
	end
	
	--Check if all dependencies are resolved.
	--[[if next(self.waiting_for) then
		error("After starting all Apps, there is still someone waiting for the start of "..(next(self.waiting_for)))
	end]]

	dynawa.window_manager:show_default()
end

function class:is_switchable(app)
	assert(app.is_app)
	for i, id in ipairs(dynawa.settings.switchable) do
		if app.id == id then
			return true
		end
	end
	return false
end

--Is this the "required" App (whose autostart cannot be disabled)?
function class:is_required(app)
	assert(app.is_app)
	for i,fname in ipairs(self.required_apps) do
		if app.filename == fname then
			return true
		end
	end
	return false
end

function class:can_be_switchable(app)
	--The App must override its inherited "switching_to_front" method in order to be switchable
	assert(app.is_app)
	return (app.switching_to_front ~= assert(Class.App.switching_to_front))
end

local function get_sd_apps_except(dirname,running,result)
	local dir = assert(dynawa.file.dir_stat(dirname))
	dynawa.busy()
	for fname,size in pairs(dir) do
		if size == "dir" then
			get_sd_apps_except(dirname..fname.."/",running,result)
		else
			local id = dirname..fname
			if not running[id] then
				if id:match("_app%.lua$") then
					--log("App: "..id)
					table.insert(result,id)
				end
			end
		end
	end
	return result
end

--Gets filenames of all SD card apps which are NOT CURRENTLY RUNNING
function class:sd_card_apps()
	local running = {}
	for id, app in pairs(self.all_apps) do
		assert(not running[app.filename])
		running[app.filename] = app
	end
	local dir = "/"
	local apps = get_sd_apps_except(dir,running,{})
	return apps
end

function class:enable_autostart(app)
	assert(app.is_app)
	assert(not self:is_autostarting(app))
	table.insert(dynawa.settings.autostart,app.filename)
	--table.insert(dynawa.settings.switchable,app.id)
	dynawa.file.save_settings()
end

function class:disable_autostart(app)
	assert(app.is_app)
	assert(self:is_autostarting(app))
	for i,fname in ipairs(dynawa.settings.autostart) do
		if fname == app.filename then
			table.remove(dynawa.settings.autostart,i)
			break
		end
	end
	for i,id in ipairs(dynawa.settings.switchable) do
		if id == app.id then
			table.remove(dynawa.settings.switchable,i)
			break
		end
	end
	dynawa.file.save_settings()
end

return class

