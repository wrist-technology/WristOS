dynawa.version = {wristOS="0.9", settings_revision = 101106}

dynawa.dofile = function(...)
	dynawa.busy()
	return dofile(...)
end

local busy_count = assert(dynawa.bitmap.from_png_file(dynawa.dir.sys.."busy_anim.png"),"Cannot load busy animation")
local busy_bitmaps = {}
local busy_last = 0
for i = 0,3 do
	busy_bitmaps[i] = dynawa.bitmap.copy(busy_count,0,32*i,54,32)
end
busy_count = 0
dynawa.busy = function(percentage) --todo: Rewrite!
	local ticks = dynawa.ticks()
	if ticks - busy_last > 200 then
		dynawa.is_busy = true
		busy_count = (busy_count + 1) % 4
		busy_last = ticks
		--start + 4, wide 46, high 8
		dynawa.bitmap.show_partial(busy_bitmaps[busy_count],nil,nil,nil,nil,53,48)
		--log("Busy -> activity")
		if dynawa.app_manager then
			local sandman = dynawa.app_manager:app_by_id("dynawa.sandman")
			if sandman then
				sandman:activity()
			end
		end
		if percentage then
			local prog1 = math.floor(percentage * 46)
			if prog1 == 0 then
				prog1 = 1
			end
			dynawa.bitmap.show_partial(dynawa.bitmap.new(prog1,8,0,255,0),nil,nil,nil,nil,57,68)
		end
	end
end

local _unique_id = 0
dynawa.unique_id = function()
	_unique_id = _unique_id + 1
	return (":".._unique_id)
end

--FILE + serialization + global settings init
dynawa.dofile(dynawa.dir.sys.."file.lua")

dynawa.settings = dynawa.file.load_data(dynawa.dir.sys.."settings.data")
if not dynawa.settings or dynawa.settings.revision < dynawa.version.settings_revision or dynawa.settings.revision > 999999 then
	dynawa.settings = {
		revision = dynawa.version.settings_revision,
		default_font = "/_sys/fonts/default10.png",
		display = {brightness = 2, autosleep = 0},
		autostart = {"/_sys/apps/clock/clock_app.lua","/_sys/apps/inbox/inbox_app.lua", "/_sys/apps/bluetooth_apps/dyno/dyno_bt_app.lua","/_sys/apps/call_manager/call_manager_app.lua","/_sys/apps/http_request/http_request_app.lua"},
		switchable = {"dynawa.clock","dynawa.inbox","dynawa.bluetooth_manager"},
		gestures = {enabled = true},
	}
	dynawa.file.save_settings()
end

--DISPLAY + BITMAP init
dynawa.dofile(dynawa.dir.sys.."bitmap.lua")

--Classes
dynawa.dofile(dynawa.dir.sys.."classes/init.lua")

dynawa.devices = {}
--#todo DeviceNodes
dynawa.devices.buttons = Class.Buttons()
dynawa.devices.display = {size = {w = 160, h = 128}, flipped = false}
dynawa.devices.display.power = assert(dynawa.x.display_power)
dynawa.devices.display.power(1)
dynawa.devices.display.brightness = assert(dynawa.x.display_brightness)
dynawa.devices.display.brightness(assert(dynawa.settings.display.brightness))

dynawa.devices.timers = Class.Timers()

dynawa.devices.vibrator = Class.Vibrator()

dynawa.devices.battery = Class.Battery()

dynawa.devices.bluetooth = Class.Bluetooth()

dynawa.devices.accelerometer = Class.Accelerometer()

dynawa.app_manager = Class.AppManager()

local hw_vectors = {}
hw_vectors.button_down = function(event)
	dynawa.devices.buttons.raw:generate_event(event)
end

hw_vectors.button_up = hw_vectors.button_down

hw_vectors.button_hold = hw_vectors.button_down

hw_vectors.timer_fired = function (event)
	local handle = assert(event.handle,"HW message of type timer_fired has no handle")
	dynawa.devices.timers:dispatch_timed_event(handle)
end

hw_vectors.bluetooth = function (event)
	dynawa.devices.bluetooth:handle_hw_event(event)
end

hw_vectors.battery = function (event)
	dynawa.devices.battery:handle_hw_event(event)
end

hw_vectors.accel = function (event)
	dynawa.devices.accelerometer:handle_hw_event(event)
end

_G.private_main_handler = function(hw_event)
	--log(tostring(hw_event.type))

	local handler = hw_vectors[hw_event.type]
	if handler then
		handler(hw_event)
	else
		log("No handler found for hw event '"..hw_event.type.."', ignored")
        log ("-----Unknown HW event:")
        hw_event.type = nil
        for k,v in pairs(hw_event) do
            log (k.." = "..tostring(v))
        end
        log("-----------")
	end
	dynawa.window_manager:update_display()
end

dynawa.app_manager:start_everything()

