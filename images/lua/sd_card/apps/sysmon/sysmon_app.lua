app.name = "System Monitor"
app.id = "dynawa.sysmon"

function app:display(bitmap, x, y)
	assert(bitmap and x and y)
	self.window:show_bitmap_at(bitmap, x, y)
end

function app:update(message)
	if not self.window.in_front then
		return
	end
	local stats = dynawa.x.sys_stats();
	stats.Lua_alloc = collectgarbage("count")*1024

	local x = 10
	local y = 10
	for k, v in pairs(stats) do
		local txtbmp = dynawa.bitmap.text_line(string.format("%s: %s", k, v),"/_sys/fonts/default10.png") 
		self:display(self.bmp_blank,x,y) 
		self:display(txtbmp,x,y) 
		y = y + 12
	end
		
	dynawa.devices.timers:timed_event{delay = 1000, receiver = self}
end

function app:handle_event_timed_event(event)
	self:update(event)
end

function app:switching_to_back()
	self.window:pop()
end

function app:switching_to_front()
	if not self.window then
		self.window = self:new_window()
		self.window:fill()
	end
	self.window:push()
    self:update()
end

function app:gfx_init()
     self.bmp_blank = dynawa.bitmap.new(150, 10) 
end

function app:going_to_sleep()
	return "remember"
end

function app:start()
	self:gfx_init()
end

