app.name = "System Clock"
app.id = "dynawa.clock"
app.window = nil
local fonts, icons

function app:start()
	self:gfx_init()
	self.window = self:new_window()
	self.window:fill()
	dynawa.app_manager:after_app_start("dynawa.inbox", function(inbox)
		inbox.events:register_for_events(self)
		inbox:broadcast_update()
	end)
	dynawa.devices.battery:register_for_events(self)
	dynawa.devices.bluetooth:register_for_events(self)
	local dyno = dynawa.app_manager:app_by_id("dynawa.dyno")
	dynawa.app_manager:after_app_start("dynawa.dyno", function(dyno)
		dyno.events:register_for_events(self,function(ev) return (ev.type == "dyno_status_changed") end)
		dyno:status_changed()
	end)
end

function app:display(bitmap, x, y)
	assert(bitmap)
	assert(x)
	assert(y)
	self.window:show_bitmap_at(bitmap, x, y)
end

function app:small_print(chars, x)
	local width, height = 13, 25
	local y = 127 - height
	for i, char in ipairs(chars) do
		if char >= 0 then --negative char == space
			self:display(fonts.small[char], x, y)
		end
		x = x + width + 2
	end
	return x
end

function app:render_date(time)
	self:small_print({time.wday * 2 + 10, time.wday * 2 + 11}, 1) --day of week
	local day1 = math.floor(time.day / 10)
	local day2 = time.day % 10

	local month1 = 0 --space
	local month2 = time.month % 10
	if time.month >= 10 then
		month1 = 1
	end

	local year1 = math.floor((time.year % 100) / 10)
	local year2 = time.year % 10

	self:small_print({day1, day2, 10}, 42)
	self:small_print({month1, month2, 10}, 78)
	self:small_print({11, year1, year2}, 161 - (3*15))
end

function app:render(time, full)
	if full == "no_time" then
		render_date(time)
		return
	end
	local top = 42
	local sec1 = math.floor(time.sec / 10)
	local sec2 = time.sec % 10
	local mm_hh = full

	self:display(fonts.medium[sec2], 160 - 17, top)
	self:display(fonts.dot,58,40+11)
	self:display(fonts.dot,58,40+31)
	if full or sec2 == 0 then
		self:display(fonts.medium[sec1], 160 - 17 - 18, top)
		if sec1 == 0 then
			mm_hh = true
		end
	end
	
	if mm_hh or full then
		local min1 = math.floor(time.min / 10)
		local min2 = time.min % 10
		local hour1 = math.floor(time.hour / 10)
		local hour2 = time.hour % 10
		self:display(fonts.large[hour1], 0, top)
		self:display(fonts.large[hour2], 27, top)
		self:display(fonts.large[min1], 69, top)
		self:display(fonts.large[min2], 96, top)
		if time.hour + time.min == 0 then --Midnight
			full = true
		end
	end
	
	if full then
		self:render_date(time)
	end
	
end

function app:remove_dots(message)
	if not self.window.in_front then
		return
	end
	local black = dynawa.bitmap.new(5,5,0,0,0)
	self.window:show_bitmap_at(black, 58,40+11)
	self.window:show_bitmap_at(black, 58,40+31)
end

function app:tick(message)
	if (self.run_id ~= message.run_id) then
		return
	end
	local sec,msec = dynawa.time.get()
	--log("Clock time before clock render: "..sec.." / "..msec)
	self:render(os.date("*t",sec), message.full_render)
	local sec2,msec2 = dynawa.time.get()
	--log("After clock render: "..sec2.." / "..msec2)
	local when = 1000 - msec2
	if message.full_render == "no_time" then
		message.full_render = true
	elseif message.full_render then
		message.full_render = nil
	end
	--log("when = "..when)
	dynawa.devices.timers:timed_event{delay = when, receiver = self, method = "tick", run_id = self.run_id, full_render = message.full_render}
	if when > 700 then
		dynawa.devices.timers:timed_event{delay = 300, receiver = self, method = "remove_dots"}
	end
end

function app:handle_event_timed_event(event)
	self[event.method](self,event)
end

function app:switching_to_back()
	self.run_id = nil
	log("Clock switching to back")
	Class.App.switching_to_back(self)
end

function app:switching_to_front()
	self.run_id = dynawa.unique_id()
	self.window:push()
	self:tick{run_id = self.run_id, full_render = true}
	self:handle_event_battery_status(dynawa.devices.battery:status())
end

function app:gfx_init()
	local bmap = assert(dynawa.bitmap.from_png_file(self.dir.."digits.png"))
	fonts={small={},medium={},large={}}
	local b_copy = dynawa.bitmap.copy
	for i=0,9 do
		fonts.large[i] = b_copy(bmap,i*26,70,24,47)
	end
	for i=0,9 do
		fonts.medium[i] = b_copy(bmap,i*18,30,16,31)
	end
	for i=0,25 do
		fonts.small[i] = b_copy(bmap,i*15,0,13,25)
	end
	fonts.dot = b_copy(bmap,0,65,5,5)
	fonts.black = b_copy(bmap,5,65,5,5)
	bmap = assert(dynawa.bitmap.from_png_file(self.dir.."notify_icons.png"))
	icons = {bitmaps = {}, state = {}, waiting = true}
	for i,id in ipairs {"email","sms","call","calendar"} do
		icons.bitmaps[id] = b_copy(bmap, i*25 - 25, 0, 25, 25)
		icons.state[id] = 0
	end
	for i, id in ipairs{"battery","battery_critical","battery_charging"} do
		icons.bitmaps[id] = b_copy(bmap, i*21 - 21, 25, 21, 40)
	end
	for i, id in ipairs{"bt_red","bt_green","dyno_red", "dyno_orange", "dyno_white"} do
		icons.bitmaps[id] = b_copy(bmap, i*21 - 21, 65, 21, 20)
	end
	icons.bitmaps.bt_black = dynawa.bitmap.new(21,20,0,0,0)
	icons.bitmaps.dyno_black = icons.bitmaps.bt_black
end

function app:handle_event_battery_status(event)
	--log("Clock received battery status update")
	if not self.window.in_front then
		return
	end
	local x,y = 139,0
	if event.critical then
		self.window:show_bitmap_at(icons.bitmaps.battery_critical,x,y)
	elseif event.charging then
		self.window:show_bitmap_at(icons.bitmaps.battery_charging,x,y) --#Test only
	else
		self.window:show_bitmap_at(icons.bitmaps.battery,x,y)
		local full = math.floor (event.percentage/100 * 29 + 0.5)
		if full > 0 then
			local fullbmp = dynawa.bitmap.new(13,full,255,0,0)
			self.window:show_bitmap_at(fullbmp, x + 4, y + 36 - full)
		end
	end
	
	--Display voltage and charging values
	local txtbmp = dynawa.bitmap.text_line(string.format("%d%+d",event.voltage,event.current),"/_sys/fonts/default7.png")
	x = 124
	y = 81
	local blank = dynawa.bitmap.new(36,10)
	self.window:show_bitmap_at(blank,x,y)
	self.window:show_bitmap_at(txtbmp,x,y)
end

function app:handle_event_inbox_updated(event)
	local icon_width = 25
	local folders = event.folders
	local new_state = {}
	local do_update = false
	for i, id in ipairs {"call","sms","email","calendar"} do
		local new = 0
		for j, msg in ipairs(folders[id]) do
			if not msg.read then
				new = new + 1
			end
		end
		new_state[id] = new
		if new ~= icons.state[id] then
			do_update = true
		end
	end
	if not do_update then
		return
	end
	icons.state = new_state
	local blank = dynawa.bitmap.new(100,40)
	self.window:show_bitmap_at(blank,0,0)
	local x_pos = 0
	for i, id in ipairs {"sms","email","calendar","call"} do
		if icons.state[id] > 0 then
			self.window:show_bitmap_at(icons.bitmaps[id], x_pos, 0)
			if icons.state[id] > 1 then
				local number = dynawa.bitmap.text_lines{text = icons.state[id], font = "/_sys/fonts/default15.png", width = icon_width, center = true, color = {255,0,0}}
				self.window:show_bitmap_at(number, x_pos, 25)
			end
			x_pos = x_pos + icon_width
		end
	end
end

function app:handle_event_gesture_sleep()
	dynawa.app_manager:app_by_id("dynawa.sandman"):sleep()
end

function app:handle_event_bluetooth(event)
	if event.subtype == "started" then
--		log("Clock: bt started")
		self:display_bt_icon("red")
	elseif event.subtype == "stopped" then
--		log("Clock: bt stopped")
		self:display_bt_icon("black")
	end
end

function app:handle_event_dyno_status_changed(event)
	--log("clock: dyno status changed")
	local acts = assert(event.activities)
	local connecting = false
	for bdaddr, act in pairs(acts) do
		--log("act.status in clock = "..act.status)
		if act.status == "connected" then
			self:display_dyno_icon("red")
			return
		end
		if act.status == "connecting" or act.status == "waiting_for_reconnect" or act.status == "finding_service" then
			connecting = true
		end
	end
	if connecting then
		self:display_dyno_icon("orange")
	else
		self:display_dyno_icon("black")
	end
end

function app:display_bt_icon(icon)
	local bmp = assert(icons.bitmaps["bt_"..icon])
	self.window:show_bitmap_at(bmp,118,0)
end

function app:display_dyno_icon(icon)
	--log("******* Clock Dyno icon = "..icon)
	local bmp = assert(icons.bitmaps["dyno_"..icon])
	self.window:show_bitmap_at(bmp,118,20)
end

