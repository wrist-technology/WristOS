local class = Class("Window")
class.is_window = true
class.bitmap = false

class.size = {w=160,h=128}

function class:_init(str)
	self.id = dynawa.unique_id()
	if str then
		self.name = str.." "..self.id
	end
	self:force_full_update()
	dynawa.window_manager:register_window(self)
	self.size = assert(dynawa.devices.display.size)
	--log("Created "..self)
end

function class:_del()
	dynawa.window_manager:unregister_window(self)
	if self.menu  then
		self.menu.window = false
		self.menu:_delete()
	end
	self.bitmap = false
end

function class:show_bitmap(bitmap)
	local w,h = dynawa.bitmap.info(bitmap)
	assert(w == self.size.w and h == self.size.h, "Bitmap is not fullwindow")
	self.bitmap = bitmap
	self:force_full_update()
end

function class:fill(color)
	if not color then
		color = {0,0,0}
	end
	self:show_bitmap(dynawa.bitmap.new(self.size.w, self.size.h, color[1], color[2], color[3]))
end

function class:show_bitmap_at(bitmap,x,y)
	assert(x and y)
	assert(self.bitmap, "show_bitmap_at() called on empty bitmap") --#todo Default background??
	dynawa.bitmap.combine(self.bitmap, bitmap, x, y)
	if self.updates.full then
		return
	end
	local w,h = dynawa.bitmap.info(bitmap)
	--Add region for show_partial
	table.insert(self.updates.regions,{x = x, y = y, w = w, h = h})
	self.updates.pixels_remain = self.updates.pixels_remain - w * h
	if #self.updates.regions >= self.updates.max_regions or self.updates.pixels_remain <= 0 then
		self.updates.full = true
	end
end

function class:allow_partial_update()
	self.updates = {regions = {}, max_regions = 9, 
	pixels_remain = math.floor(self.size.w * self.size.h * 0.95)}
end

function class:force_full_update()
	self.updates = {full = true}
end

function class:handle_event_button(ev)
	if self.menu then
		return self.menu:handle_event_button(ev)
	end
	assert(self.app)
	self.app:handle_event_button(ev)
end

function class:push()
	return dynawa.window_manager:push(self)
end

function class:pop()
	local popped = dynawa.window_manager:pop()
	if popped ~= self then
		error("Popped window is "..popped..", not "..self)
	end
	return self
end

function class:remove_from_stack()
	dynawa.window_manager:remove_from_stack(self)
end

function class:overlaid_by(win)
	--This window was in front but is now overlaid by window 'win'
	--Do nothing
end

--[[function class:to_front()
	return dynawa.window_manager:window_to_front(self)
end]]

--[[function class:you_are_now_in_front()
	return self.app:window_in_front(self)
end]]

return class

