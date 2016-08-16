app.name = "Auto WakeUp"
app.id = "dynawa.auto_wakeup"

--[[
	INFO:
	This App automatically wakes up the watch in specified interval.
	When you want to look supercool, just set some interesting animated App (for example "9bit clock") 
	as your default App and activate Auto WakeUp. There is no need to make Auto WakeUp auto-startable if you
	don't want the effect to last after reset.
	
	Auto WakeUp does not mess with "Display autosleep" in any way. Specifically, if the autosleep interval is equal
	or longer than Auto WakeUp interval, they cancel each other out and the watch never goes to sleep by itself at all.
	
	Note that the wake up event always happens at rounded-up times. For example with interval of "2" minutes, the wake up
	event happens EXACTLY on :00, :02, :04, :06 etc. on every hour. This means that when you set the interval to
	"2 minutes", the first wake up will wery probably happen sooner than in 2 minutes.
]]


function app:wake_after(int) --In SECONDS!
	if int == 0 then
		self.run_id = nil
		return
	end
	self.run_id = dynawa.unique_id()
	local delay = (int - (os.time() % int)) * 1000
	dynawa.devices.timers:timzed_event{delay = delay, run_id = self.run_id, receiver = self}
end

function app:handle_event_timed_event(event)
	if assert(event.run_id) ~= self.run_id then
		return
	end
	local sandman = assert(dynawa.app_manager:app_by_id("dynawa.sandman"), "Sandman not running")
	sandman:wake_up()
	self:wake_after(self.prefs.interval)
end

function app:switching_to_front()
	self:show_menu()
end

function app:show_menu()
	local menudef = {banner = "Change Auto WakeUp value (now "..self.prefs.interval_txt..")", items = {}}
	local values = {{0,"Disabled"}, {10,"10 seconds"}, {20,"20 seconds"}, {30, "30 seconds"}, {60, "1 minute"}, {120, "2 minutes"}, {180, "3 minutes"}, {300, "5 minutes"}, {600,"10 minutes"}, {1200, "20 minutes"}, {1800, "30 minutes"}, {3600, "1 hour"}}
	for i, tuple in ipairs(values) do
		table.insert(menudef.items,{text = tuple[2], value = tuple[1]})
	end
	local menuwin = self:new_menuwindow(menudef)
	menuwin:push()
end

function app:menu_item_selected(args)
	--log("Auto Wakeup menu item selected")
	dynawa.window_manager:pop()
	self.prefs.interval = assert(args.item.value)
	self.prefs.interval_txt = assert(args.item.text)
	self:show_menu()
	self:wake_after(self.prefs.interval)
	dynawa.popup:info("Auto WakeUp interval set to: "..self.prefs.interval_txt)
	self:save_data(self.prefs)
	
end

function app:start()
	self.prefs = self:load_data() or {interval = 0, interval_txt = "Disabled"}
	self:wake_after(self.prefs.interval)
end

