app.name = "Window Manager"
app.id = "dynawa.window_manager"

function app:start()
	dynawa.window_manager = self
	self._windows = {}
	self.front_window = false
	self._last_displayed_window = false
	self.stack = {}
	dynawa.devices.buttons:register_for_events(self)
	dynawa.devices.buttons.virtual:register_for_events(self)
end

function app:show_default()
	local id = dynawa.settings.switchable[1]
	if not id then --There are no switchables defined, show SuperMan instead
		dynawa.superman:switching_to_front()
	else
		local app = dynawa.app_manager:app_by_id(id)
		assert(app, "This app is not running: "..id)
		--log("Switching to front: "..app)
		app:switching_to_front()
	end
	assert(self:peek(), "Default App did not put any Window on stack")
end

function app:push(x)
	assert(x.is_window)
	for i,w in ipairs(self.stack) do
		if w==x then
			error(x.." is already present in window stack")
		end
	end
	if self.stack[1] then
		self.stack[1].in_front = nil
	end
	table.insert(self.stack,1,x)
	--self:window_to_front(x)
	x.in_front = true
	log("Pushed "..x..", "..#(self.stack).." now on stack")
	if #self.stack >= 2 then
		self.stack[2]:overlaid_by(x)
	end
end

--For debugging only, #todo remove
function app:log_windows()
	local n = 0
	local ids = {}
	for win,id in pairs(self._windows) do
		assert(win.id == id)
		n = n + 1
		table.insert(ids,tostring(win))
	end
	log(n.." windows registered: "..table.concat(ids,", "))
end

function app:pop()
	local x = assert(table.remove(self.stack,1),"Nothing to pop from stack")
	assert(x.is_window, "Should be window")
	assert(x.in_front, "Should be front window")
	log("Popped "..x..", "..#(self.stack).." remain on stack")
	x.in_front = nil
	if self.stack[1] then
		self.stack[1].in_front = true
	end
	--self:log_windows()
	return x
end

function app:remove_from_stack(win)
	assert(win.is_window,"Should be window")
	if win.in_front then
		return win:pop()
	end
	for i,w in ipairs(self.stack) do
		--log("stack "..i.." = "..w)
		if w == win then
			table.remove(self.stack, i)
			log(w .. " removed from stack position "..i)
			return w
		end
	end
	error(win.." not found in window stack")
end

--This is a powerful but potentially dangerous method that pops all menuwindows from top of the stack
--and automatically deletes (invalidates!) all of them (i.e. they should not be referenced from anywhere else at this point!).
--It stops at first window with no menu and returns this window (or nil, if there is no such window).
function app:pop_and_delete_menuwindows()
	while true do
		local window = self:peek()
		if not window then
			return nil
		end
		if window.menu then
			self:pop():_delete()
		else
			return window
		end
	end
end

function app:peek()
	return (self.stack[1])
end

function app:register_window(window)
	assert (not self._windows[window], "Window already registered")
	self._windows[window] = window.id or true
	--log("Registered "..window)
end

function app:unregister_window(window)
	assert (self._windows[window], "Window not registered")
	self._windows[window] = nil
	--log("Unregistered "..window)
end

function app:update_display()
	local window = self:peek()
	if not window then --No windows in stack
		local sandman = dynawa.app_manager:app_by_id("dynawa.sandman")
		if sandman and sandman.sleeping then
			return
		end
		self:show_default()
		window = assert(self:peek())
	end
	if window.menu and window.menu:requires_render() then
		log("Re-rendering menu")
		window.menu:render()
	end
	if dynawa.is_busy then
		dynawa.is_busy = nil
		window.updates.full = true
	end
	if window.updates.full or self._last_displayed_window ~= window then
		--log("showing window "..window)
		if not window.bitmap then
			window.bitmap = dynawa.bitmap.new(dynawa.devices.display.size.w,dynawa.devices.display.size.h,255,0,0)
			dynawa.bitmap.combine(window.bitmap, dynawa.bitmap.text_lines{text=window.." has no bitmap!"},1,20)
		end
		dynawa.bitmap.show(window.bitmap,dynawa.devices.display.flipped)
	else
		for _, region in ipairs(window.updates.regions) do
			dynawa.bitmap.show_partial(window.bitmap,region.x,region.y,region.w,region.h,region.x,region.y,
					dynawa.devices.display.flipped)
			--log("Display_update "..region.x..","..region.y..","..region.w..","..region.h)
		end
	end
	window:allow_partial_update()
	self._last_displayed_window = window
end

function app:handle_event_button(event)
	local win = self:peek()
	if win then
		win:handle_event_button(event)
	end
end

function app:handle_event_do_switch()
	local win0 = self:peek()
	self:stack_cleanup()
	local switchable = dynawa.settings.switchable
	if (not win0) or (#switchable <= 1) then
		return self:show_default()
	end
	local app1 = assert(win0.app)
	local index1 = nil
	for i,id in ipairs(switchable) do
		if id == app1.id then
			index1 = i
			break
		end
	end
	if not index1 then --The active app is not present in 'switchable'
		return self:show_default()
	end
	local index2 = index1 + 1
	if index2 > #switchable then
		index2 = 1
	end
	local app2 = dynawa.app_manager:app_by_id(switchable[index2])
	app2:switching_to_front()
end

function app:handle_event_do_superman()
	self:stack_cleanup()
	dynawa.superman:switching_to_front()
end

--Calls 'switching_to_back' on all apps referenced in stack, effectively clearing the stack.
function app:stack_cleanup()
	local last = "?"
	while true do
		local peekwin = self:peek()
		if not peekwin then
			return
		end
		if peekwin.app == last then
			error(peekwin.app.." did not remove all its windows from stack during 'switching_to_back', "..peekwin.." is on top")
		end
		last = peekwin.app
		last:switching_to_back(peekwin)
	end
end

function app:handle_event_do_menu()
	local win0 = self:peek()
	if win0 then
		return win0.app:handle_event_do_menu(win0)
	end
end

return app

