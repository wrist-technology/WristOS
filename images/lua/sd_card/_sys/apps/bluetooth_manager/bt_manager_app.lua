app.name = "Bluetooth Manager"
app.id = "dynawa.bluetooth_manager"

function app:start()
	dynawa.bluetooth_manager = self
	self.events = Class.EventSource("bluetooth_manager")
	self.prefs = self:load_data() or {devices = {}}
	self.hw = assert(dynawa.devices.bluetooth)
	self.hw:register_for_events(self)
	self.hw_status = "off"
end

local function all_bt_apps_iterator(apps, key0)
	--log("iterating")
	local key, app = next(apps,key0)
	if key then
		if app:_class() == Class.BluetoothApp then
			--log("matches:"..key)
			return key, app
		else
			return all_bt_apps_iterator(apps, key)
		end
	end
end

function app:all_bt_apps() --iterator
	return all_bt_apps_iterator, assert(dynawa.app_manager.all_apps), nil
end

local btapps_item_selected = function(_self,args)
	args.menu.window:pop():_delete()
	local app_id = assert(args.item.value.app_id)
	local app = assert(dynawa.app_manager:app_by_id(app_id))
	app:handle_event_do_menu()
end

function app:menu_action_bt_apps(args)
	local menudesc = {banner = "Bluetooth Apps:", item_selected = btapps_item_selected}
	local items = {}
	for app_id, app in self:all_bt_apps() do
		local status = assert(app:status_text())
		local desc = app.name
		if status ~= "" then
			desc = desc ..  " ("..status..")"
		end
		table.insert(items,{text = desc, value = {app_id = app_id}})
	end
	table.sort(items, function (a,b)
		return (a.text < b.text)
	end)
	menudesc.items = items
	self:new_menuwindow(menudesc):push()
end

local btactivities_item_selected = function(_self,args)
end

function app:menu_action_bt_activities(args)
	local menudesc = {banner = "Bluetooth Activities:", items = {}}
	local apps = {}
	for app_id, app in self:all_bt_apps() do
		local app_item = {name = app.name}
		app_item.activity_items = app:activity_items()
		if next(app_item.activity_items) then
			table.insert(apps,app_item)
		end
	end
	table.sort(apps, function (a,b)
		return (a.name < b.name)
	end)
	for i, app in ipairs(apps) do
		table.insert(menudesc.items,{text = "- "..app.name..":"})
		for j, act in ipairs(app.activity_items) do
			table.insert(menudesc.items, act)
		end
	end
	self:new_menuwindow(menudesc):push()
end

function app:switching_to_front()
	local menu = {
		banner = "Bluetooth manager",
		items = {
			{
				text = "BT on", value = {jump = "bt_on"},
			},
			{
				text = "BT off", value = {jump = "bt_off"},
			},
			{
				text = "Bluetooth Apps", value = {jump = "bt_apps"},
			},
			{
				text = "Bluetooth Activities", value = {jump = "bt_activities"},
			},
--[[			{
				text = "Send 'Hello'", value = {jump = "send_watch_hello"},
			},
			{
				text = "Send 20x Hello World (long string)", value = {jump = "send_watch_helloworld20"},
			},
			{
				text = "Send 50x Hello World (long string)", value = {jump = "send_watch_helloworld50"},
			},
			{
				text = "Send complex structure", value = {jump = "send_watch_struct"},
			},
			{
				text = "Emulate random calendar_event", value = {jump = "random_calendar_event"},
			},]]
			{
				text = "Show paired devices", value = {jump = "show_pairings"},
			},
			{
				text = "Delete all paired devices", value = {jump = "delete_pairings"},
			},
		},
	}
	local menuwin = self:new_menuwindow(menu)
	menuwin:push()
end

--[[
function app:menu_cancelled(menu)
	menu.window:pop()
end
]]

function app:send_watch(args)
	local data = assert(args.data)
	local app = dynawa.app_manager:app_by_id("dynawa.bt.watch")
	app:send_data_test(data)
end

function app:menu_action_send_watch_hello()
	self:send_watch{data="Hello"}
end

function app:menu_action_send_watch_helloworld20()
	local str = {}
	for i = 1, 20 do 
		table.insert(str,"Hello world "..i.."!")
	end
	self:send_watch{data=table.concat(str), " "}
end

function app:menu_action_send_watch_helloworld50()
	local str = {}
	for i = 1, 50 do 
		table.insert(str,"Hello world "..i.."!")
	end
	self:send_watch{data=table.concat(str), " "}
end

function app:menu_action_random_calendar_event()
	local event = {command = "calendar_event", description = "Lunch "..math.random(99999), location = "Hilton hotel, Berlin", details = "Bring guns & ammo", contact_name = "John Smith", time = os.time() + math.random(60*60*20) - 5*60*60}
	local app = dynawa.app_manager:app_by_id("dynawa.inbox")
	app:handle_event_from_phone{type = "from_phone", data = event}
end
function app:menu_action_send_watch_struct()
	local data = {string = "Hello world", number = 666, TRUE = true, FALSE = false, array = {1,2,"three",4,5}, 
			subhash = {key1 = "val1", key2 = "val2"}}
	self:send_watch{data=data}
end

function app:menu_item_selected(args)
	local value = args.item.value
	if not value then
		return
	end
	self["menu_action_"..value.jump](self,value)
end

function app:menu_action_delete_pairings()
	for bdaddr, device in pairs(self.prefs.devices) do
		dynawa.busy()
		self.events:generate_event{type="removed_paired_device", device={link_key = assert(device.link_key), name = assert(device.name), bdaddr = bdaddr}}
		self.prefs.devices[bdaddr] = nil
	end
	assert(not next(self.prefs.devices), "Device table should be empty")
	self:save_data(self.prefs)
	dynawa.popup:info("All pairings deleted")
end

function app:menu_action_show_pairings()
	local menudesc = {banner = "Paired devices:",items = {}}
	for bdaddr, device in pairs(self.prefs.devices) do
		table.insert(menudesc.items,{text = device.name})
	end
	table.sort(menudesc.items, function(a,b)
		return (a.text < b.text)
	end)
	self:new_menuwindow(menudesc):push()
end

function app:menu_action_bt_on(args)
	if self.hw_status ~= "off" then
		dynawa.popup:open({style="error", text="Bluetooth is currently not OFF so it cannot be turned ON"})
		return
	end
	self.hw_status = "opening"
	log("Opening BT hardware NOW")
	--self.hw.cmd:open()
	--self.hw.cmd:open(Class.Bluetooth.cod.WEARABLE)
	self.hw.cmd:open(Class.Bluetooth.cod.HANDSFREE)
	log("Opened BT hardware")
end

function app:menu_action_bt_off(args)
	if self.hw_status ~= "on" then
		dynawa.popup:open({style="error", text="Bluetooth is currently not ON so it cannot be turned OFF"})
		return
	end
	for app_id, app in self:all_bt_apps() do
		app:handle_bt_event_turning_off()
	end
	self.hw_status = "closing"
	self.hw.cmd:close()
end

function app:handle_event_bluetooth(event)
--[[	log("---BT event received:")
	for k,v in pairs(event) do
		if k ~= "source" and k ~= "type" then
			log(tostring(k).." = "..tostring(v))
		end
	end]]
	self["handle_bt_event_"..event.subtype](self,event)
end

function app:handle_bt_event_started(event)
	self.hw_status = "on"
	log("BT on")
	for app_id, app in self:all_bt_apps() do
		app:handle_bt_event_turned_on()
	end
end

function app:handle_bt_event_stopped(event)
	if self.hw_status == "restarting" then
		self.hw_status = "opening"
		self.hw.cmd:open()
		return
	end
	self.hw_status = "off"
	log("BT off")
end

function app:handle_bt_event_link_key_req(event)
	local bdaddr = assert(event.bdaddr)
	local link_key
	if self.prefs.devices[bdaddr] then
		link_key = self.prefs.devices[bdaddr].link_key
	end
	if link_key then
		self.hw.cmd:link_key_req_reply(bdaddr, link_key)
	else
		--error("I don't have the link key!")
		self.hw.cmd:link_key_req_neg_reply(bdaddr)
	end
end

function app:handle_bt_event_link_key_not(event)
	local bdaddr = assert(event.bdaddr)
	local link_key = assert(event.link_key)
	log("Link_key_not")
	if not self.prefs.devices[bdaddr] then
		self.prefs.devices[bdaddr] = {name = "MAC "..Class.Bluetooth:mac_string(bdaddr), bdaddr = bdaddr}
		--log("name:"..name)
	end
	if self.prefs.devices[bdaddr].link_key ~= link_key then
		self.prefs.devices[bdaddr].link_key = link_key
		log("link key")
		self.events:generate_event{type="new_paired_device",device={link_key = link_key, name = name, bdaddr = bdaddr}}
		self:save_data(self.prefs)
        -- test
		self.hw.cmd:remote_name_req(bdaddr)
	end
end

function app:handle_bt_event_remote_name(event)
	local bdaddr = assert(event.bdaddr)
	local name = assert(event.name)
	log("Got remote_name: "..name)
	local dev = self.prefs.devices[bdaddr]
	if dev then
		dev.name = name.." ("..Class.Bluetooth:mac_string(dev.bdaddr)..")"
		self:save_data(self.prefs)
	else
		log("mac not found")
	end
end

