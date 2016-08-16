local class = Class("Menu")
class.scroll_delay = 150
class.is_menu = true

function class:_init(desc)
	self.flags = desc.flags or {}
	self.outer_color = desc.outer_color or {99,99,255}
	self.banner = assert(desc.banner)
	if type(self.banner) == "string" then
		self.banner = {text = self.banner}
	end
	if self.banner.text then
		self.name = self.banner.text
	end
	self.items = {}
	self:clear_cache()
	if desc.item_selected then
		assert(type(desc.item_selected) == "function", "'item_selected' attribute is not a function")
		self.item_selected = desc.item_selected
	end
	for item_n, item_desc in ipairs(desc.items) do
		local menuitem = Class.MenuItem(item_desc)
		table.insert(self.items, menuitem)
		if desc.active_value and desc.active_value == menuitem.value then
			self.active_item = menuitem
		end
	end
	if not next(self.items) then
		table.insert(self.items,Class.MenuItem{text = "(No items in this menu)"})
	end
	if desc.active_item_index then
		self.active_item = assert(self.items[desc.active_item_index], "Invalid item index")
	else
		self.active_item = self.active_item or self.items[1] --can be nil if no items!
	end
end

function class:clear_cache()
	self.cache = {items = {}, outer_bitmap = false}
	if self.window then
		self.window.bitmap = false
	end
end

function class:invalidate()
	self:clear_cache()
end

function class:render()
	if self:requires_render() then
		self:clear_cache()
		self.window:show_bitmap(self:_render_outer())
		self.cache.outer_bitmap = true
	end
	self:_render_inner()
end

function class:re_render()
	self:clear_cache()
	self:render()
end

function class:requires_render()
	return not self.cache.outer_bitmap
end

function class:_show_bmp_inner_at(bmp, x, y)
	dynawa.bitmap.combine(self.cache.inner_bmp, bmp, x, y)
end

function class:active_item_index()
	return self:item_index(self.active_item)
end

function class:item_index(item)
	local act_i = 1
	while assert(self.items[act_i]) ~= item do
		act_i = act_i + 1
	end
	return act_i
end

--[[function class:remove_item(item)
	assert(item)
	if self.active_item == item then
		self.active_item = nil
	end
	for i, item0 in ipairs(self.items) do
		if item0 == item then
			table.remove(self.items, i)
			break
		end
	end
	self:invalidate()
end]]

function class:_add_above(items)
	local i = items.first - 1
	assert (i >= 1)
	local bmp, w, h = self:_bitmap_of_item(self.items[i])
	items[i] = {y = items[i+1].y - h, bitmap = bmp, h = h}
	items.first = i
end

function class:_add_below(items)
	local i = items.last + 1
	assert (i <= #self.items)
	local bmp, w, h = self:_bitmap_of_item(self.items[i])
	items[i] = {y = items[i-1].y + items[i-1].h, bitmap = bmp, h = h}
	items.last = i
end

function class:_add_above_and_below(items)
	local off = items.offset
	while items.first > 1 and items[items.first].y + off > 0 do
		self:_add_above(items)
	end
	while items.last < #self.items and items[items.last].y + items[items.last].h + off < items.inner_height do
		self:_add_below(items)
	end
end

function class:_render_inner()
	self.cache.inner_bmp =  dynawa.bitmap.new(self.cache.inner_size.w, self.cache.inner_size.h, 0,0,0)
	local margin = math.floor(dynawa.fonts[dynawa.settings.default_font].height / 2)
	local act_i = self:active_item_index()
	local above = self.above_active or 0
	local inner_size = assert(self.cache.inner_size)

	local aitembmp = self:_bitmap_of_item(self.active_item)
	local aw,ah = dynawa.bitmap.info(aitembmp)
	aitembmp = dynawa.bitmap.combine(dynawa.bitmap.new(inner_size.w,ah,0,0,99), aitembmp, 0, 0)
	local items = {first = act_i, last = act_i, offset = 0, inner_height = inner_size.h}

	if act_i == #self.items then
		above = inner_size.h - ah
	elseif above + ah + margin > inner_size.h then
		above = inner_size.h - margin - ah
	end
	
	if act_i == 1 then
		above = 0
	elseif above < margin then
		above = margin
	end
	
	items[act_i] = {bitmap = aitembmp, y = above, h = ah}
	
	self:_add_above_and_below(items)

	local space_bottom = inner_size.h - (items[items.last].y + items[items.last].h + items.offset)
	if space_bottom > 0 then
		items.offset = items.offset + space_bottom
		self:_add_above_and_below(items)
	end

	--log("space above = "..items[items.first].y + items.offset)
	if items[items.first].y + items.offset > 0 then
		items.offset = 0 - items[items.first].y
		self:_add_above_and_below(items)
	end

	local offset = items.offset
	for i = items.first, items.last do
		local item = items[i]
		if item.y + offset < inner_size.h or item.y + item.h + offset > 0 then
			self:_show_bmp_inner_at(item.bitmap, 0, item.y + offset)
		end
	end

	self.above_active = above + offset
	self.window:show_bitmap_at(self.cache.inner_bmp, self.cache.inner_at.x, self.cache.inner_at.y)
end

function class:_bitmap_of_item(menuitem)
	local bitmap = self.cache.items[assert(menuitem)]
	if not bitmap then
		--log("Rendering menu item "..menuitem)
		bitmap = menuitem:render{max_size = assert(self.cache.inner_size)}
		local w,h = dynawa.bitmap.info(bitmap)
		if not (w <= self.cache.inner_size.w and h <= self.cache.inner_size.w) then
			error("MenuItem bitmap too large: "..w.."x"..h)
		end
		self.cache.items[menuitem] = bitmap
		menuitem._w, menuitem._h = w,h
	end
	return bitmap, menuitem._w, menuitem._h
end

--render outer menu border and banner
function class:_render_outer()
	local outer_color = self.outer_color
	local display_w, display_h = self.window.size.w, self.window.size.h
	local bgbmp = dynawa.bitmap.new(display_w, display_h,unpack(outer_color))
	local banner_bmp, _, banner_h  = dynawa.bitmap.text_lines{text=assert(self.banner.text),
			font = nil, color = {0,0,0}, width = display_w - 2}
	self.cache.inner_at = {x = 2, y = 3 + banner_h}
	self.cache.inner_size = {w = display_w - 2 * self.cache.inner_at.x, h = display_h - self.cache.inner_at.y - 2}
	dynawa.bitmap.combine(bgbmp, banner_bmp, 1, 1)
	local inner_black =  dynawa.bitmap.new(self.cache.inner_size.w + 2, self.cache.inner_size.h + 2, 0,0,0)
	dynawa.bitmap.combine(bgbmp, inner_black, self.cache.inner_at.x - 1, self.cache.inner_at.y - 1)
	return bgbmp
end

function class:scroll(button)
	local ind = self:active_item_index()
	if button == "top" then
		ind = ind - 1
		if ind < 1 then
			ind = #self.items
			self.above_active = 999
			local above = 0
			for i = 1, ind do
				local w,h = dynawa.bitmap.info(self:_bitmap_of_item(self.items[i]))
				if i == ind and above + h <= self.cache.inner_size.h then
					self.above_active = above
					break
				end
				above = above + h
				if above >= self.cache.inner_size.h then
					break
				end
			end
		else
			local w,h = dynawa.bitmap.info(self:_bitmap_of_item(self.items[ind]))
			self.above_active = self.above_active - h
		end
	else --bottom
		local w,h = dynawa.bitmap.info(self:_bitmap_of_item(self.active_item))
		self.above_active = self.above_active + h
		ind = ind + 1
		if ind > #self.items then
			ind = 1
			self.above_active = 0
		end
	end
	self.active_item = self.items[ind]
	--local t0 = dynawa.ticks()
	self:_render_inner()
	--log("Menu updated in "..dynawa.ticks() - t0)
end

function class:handle_event_button(event)
	if event.action == "button_down" then
		if event.button == "top" or event.button == "bottom" then
			self:scroll(event.button)
		elseif event.button == "confirm" then
			self.active_item:selected({item_index = self:active_item_index(), menu = self, item = self.active_item})
		elseif event.button == "cancel" then
			local app = assert(self.window.app)
			app:menu_cancelled(self)
		end
	elseif event.action == "button_hold" then
		if event.button == "top" or event.button == "bottom" then
			self._scroll = event.button
			self:scroll(event.button)
			dynawa.devices.timers:timed_event{delay = self.scroll_delay, receiver = self, direction = event.button}
		end
	elseif event.action == "button_up" then
		self._scroll = nil
	end
end

function class:item_selected(args)
	assert(args.menu == self)
	assert(args.menu.window, "Menu has no window")
	return assert(args.menu.window.app, "Menu's Window has no App"):menu_item_selected(args)
end

function class:handle_event_timed_event(event)
	if self._scroll == event.direction then
		dynawa.devices.timers:timed_event{delay = self.scroll_delay, receiver = self, direction = event.direction}
		self:scroll(event.direction)
	end
end

function class:_del()
	self:clear_cache()
	if self.window then
		self.window.menu = false
		self.window:_delete()
	end
	for i, item in ipairs(self.items) do
		item:_delete()
	end
end

return class

