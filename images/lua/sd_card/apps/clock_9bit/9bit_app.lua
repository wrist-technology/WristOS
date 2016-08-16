app.name = "9Bit Clock"
app.id = "dynawa.clock_9bit"
-- Flashy clock, especially for using with Auto WakeUp
-- Use CONFIRM key to switch between different visualizations


app.numbers = {
	[0] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x,y+1}, {x+2,y+1}, {x,y+2}, {x+2,y+2}, {x,y+3}, {x+2, y+3}, {x,y+4}, {x+1,y+4}, {x+2, y+4}}
	end,
	[1] = function(x,y)
		return {{x+1,y}, {x,y+1}, {x+1,y+1}, {x+1,y+2}, {x+1, y+3}, {x, y+4}, {x+1, y+4}, {x+2, y+4}}
	end,
	[2] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x+2,y+1}, {x,y+2}, {x+1,y+2}, {x+2,y+2}, {x,y+3}, {x,y+4}, {x+1,y+4}, {x+2,y+4}}
	end,
	[3] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x+2,y+1}, {x,y+2}, {x+1,y+2}, {x+2,y+2}, {x+2,y+3}, {x,y+4}, {x+1,y+4}, {x+2,y+4}}
	end,
	[4] = function(x,y)
		return {{x,y}, {x,y+1}, {x+2,y+1}, {x,y+2}, {x+1,y+2}, {x+2,y+2}, {x+2,y+3}, {x+2,y+4}}
	end,
	[5] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x,y+1}, {x,y+2}, {x+1,y+2}, {x+2,y+2}, {x+2,y+3}, {x,y+4}, {x+1,y+4}, {x+2,y+4}}
	end,	
	[6] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x,y+1}, {x,y+2}, {x+1,y+2}, {x+2,y+2},{x,y+3}, {x+2,y+3}, {x,y+4}, {x+1,y+4}, {x+2,y+4}}
	end,
	[7] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x+2,y+1}, {x+2,y+2}, {x+2,y+3}, {x+2,y+4}}
	end,
	[8] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x,y+1}, {x+2,y+1}, {x,y+2}, {x+1,y+2}, {x+2,y+2},{x,y+3}, {x+2,y+3}, {x,y+4}, {x+1,y+4}, {x+2,y+4}}
	end,
	[9] = function(x,y)
		return {{x,y}, {x+1,y}, {x+2,y}, {x,y+1}, {x+2,y+1}, {x,y+2}, {x+1,y+2}, {x+2,y+2}, {x+2,y+3}, {x,y+4}, {x+1,y+4}, {x+2,y+4}}
	end,
}

function app:fill()
	local nsprites = #self.sprites
	for i = 0,9 do
		for j = 0,7 do
			self.window:show_bitmap_at(self.sprites[math.random(nsprites)],i*16,j*16)
		end
	end
end

function app:plot(x,y)
	self.window:show_bitmap_at(self.sprites[math.random(#self.sprites)],x*16,y*16)
	--log("plot: "..x..","..y)
end

function app:display_digit_at(num,x,y,args)
	args = args or {}
	local nsprites = #self.sprites
	local sprnum = 0
	for i,xy in ipairs(self.numbers[num](x,y)) do
		if not args.clear then
			sprnum = math.random(nsprites)
		end
		self.window:show_bitmap_at(self.sprites[sprnum], xy[1]*16, xy[2]*16)
	end
end

function app:display_2digits_at(num,x,y,args)
	self:display_digit_at(math.floor(num/10),x,y,args)
	self:display_digit_at(num % 10,x+4,y,args)
end

function app:animate_bars(desc)
	self.window:fill()
	local time = os.date("*t")

	--hours
	if time.hour >= 20 then
		self:plot(1,0)
	end
	if time.hour >= 10 then
		self:plot(0,0)
	end
	local i = 1
	for ii = 1, time.hour % 10 do
		self:plot(10-i,1)
		i = i + 1
		if i == 6 then
			i = i + 1
		end
	end
	
	--minutes
	for i = 1,math.floor(time.min / 10) do
		self:plot(i-1,3)
	end
	local i = 1
	for ii = 1, time.min % 10 do
		self:plot(10-i,4)
		i = i + 1
		if i == 6 then
			i = i + 1
		end
	end
	
	--seconds
	for i = 1,math.floor(time.sec / 10) do
		self:plot(i-1,6)
	end
	local i = 1
	for ii = 1, time.sec % 10 do
		self:plot(10-i,7)
		i = i + 1
		if i == 6 then
			i = i + 1
		end
	end	
	
	local s,ms = dynawa.time.get()
	desc.delay = 1000-ms
end

function app:animate_default(desc)
	desc.delay = 500
	local time = os.date("*t") --hour,min,sec
	if not desc.count then
		self.window:fill()
		self:display_2digits_at(time.hour,1,1)
		desc.count = 1
	else
		assert(desc.count == 1)
		self.window:fill()
		self:display_2digits_at(time.min,2,2)
		desc.count = nil
	end
end

function app:animate_negative(desc)
	desc.delay = 500
	local time = os.date("*t") --hour,min,sec
	if not desc.count then
		self:fill()
		self:display_2digits_at(time.hour,1,1,{clear = true})
		desc.count = 1
	else
		assert(desc.count == 1)
		self:fill()
		self:display_2digits_at(time.min,2,2,{clear = true})
		desc.count = nil
	end
end

function app:start_animation()
	self.run_id = dynawa.unique_id()
	self:animate{sequence = assert(self.prefs.sequence)}
end

function app:animate(desc)
	desc = self["animate_"..desc.sequence](self,desc) or desc
	dynawa.devices.timers:timed_event{delay = assert(desc.delay), animate = desc, run_id = self.run_id, receiver = self}
end

function app:handle_event_timed_event(event)
	if event.run_id ~= self.run_id then
		return
	end
	if self.window.in_front then
		self:animate(event.animate)
	else --We are deep inside window stack, don't exert yourself and try again in 500 ms
		event.delay = 500
		dynawa.devices.timers:timed_event(event)
	end
end

function app:switching_to_front()
	self:start_animation()
	self.window:push()
end

function app:change_sequence()
	local seq = assert(self.prefs.sequence)
	if seq == "default" then
		seq = "negative"
	elseif seq == "bars" then
		seq = "default"
	else
		assert(seq == "negative")
		seq = "bars"
	end
	self.prefs.sequence = seq
	self:save_data(self.prefs)
	self:start_animation()
end

function app:switching_to_back()
	getmetatable(self).switching_to_back(self)
end

function app:gfx_init()
	local bmap = assert(dynawa.bitmap.from_png_file(self.dir.."gfx.png"))
	self.sprites = {}
	for i = 1,14 do
		self.sprites[i] = dynawa.bitmap.copy(bmap, i*16 - 15, 1, 15, 15)
	end
	self.sprites[0] = dynawa.bitmap.new(15,15)
end

function app:handle_event_button(event)
	if event.action == "button_down" and event.button == "confirm" then
		self:change_sequence()
	else
		getmetatable(self).handle_event_button(self,event) --Parent's handler
	end
end

function app:start()
	self.prefs = self:load_data() or {sequence = "default"}
	self:gfx_init()
	self.window = self:new_window()
	self.window:fill()
end

function app:handle_event_do_menu()
	dynawa.popup:info("Use the CONFIRM button to change clock style.")
end

function app:handle_event_gesture_sleep()
	if self.window and dynawa.window_manager:peek() == self.window then
		--Don't go to sleep while my settings menu is displayed.
		dynawa.app_manager:app_by_id("dynawa.sandman"):sleep()
	end
end


