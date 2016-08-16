local class = Class("App")
class.is_app = true

function class:_init(id)
	self.id = assert(id)
end

function class:start(id)
end

function class:new_window()
	local win = Class.Window(self.name)
	win.app = self
	return win
end

function class:new_menuwindow(menudef)
	local win = Class.Window(self.name.." (menu)")
	win.app = self
	local menu = Class.Menu(menudef)
	menu.window = win
	win.menu = menu
	return win
end

function class:push_window(window)
	assert(window.is_window,"Not a window")
	window.app = self
	return dynawa.window_manager:push(window)
end

function class:_del()
	error("Attempt to delete an App: "..self)
end

--[[function class:window_in_front(window)
end]]

function class:switching_to_front()
	dynawa.popup:error(self.." generates no graphical output by default.")
end

--This is acceptable default for trivial Apps (one window plus zero or more standard menuwindows on top of that)
function class:switching_to_back()
	local win = dynawa.window_manager:pop_and_delete_menuwindows()
	if win then
		local popped = win:pop()
		if popped.app ~= self then
			error("The first popped non-menu window "..popped.." belongs to "..popped.app..", not to "..self)
		end
	end
	local peek = dynawa.window_manager:peek()
	assert (not peek or peek.app ~= self, "After popping one non-menu window, there are still other windows of mine on stack. You have to override the default trivial switching_to_back() App method with something more suitable for this App.")
end

function class:handle_event_button(event)
	if event.action == "button_up" and event.button == "cancel" then
		self:switching_to_back()
	end
end

function class:handle_event_do_menu()
	dynawa.popup:error("No context menu defined for "..self)
end

function class:handle_event_gesture_sleep()
	--log(self.." sleep gesture")
end

function class:going_to_sleep()
	return false
	--if this is overriden to return "remember", this app is switched to after wakeup
end

function class:menu_item_selected()
	return false
end

function class:menu_cancelled(menu)
	local peek = dynawa.window_manager:peek()
	assert (menu.window == peek, "Top menu window mismatch ("..menu.window.."/"..peek..")")
	--[[if #dynawa.window_manager.stack == 1 then --This is the ONLY window on stack
		return
	end]]
	menu.window:pop():_delete()
end

function class:load_data()
	return dynawa.file.load_data(self.dir.."my.data")
end

function class:save_data(data)
	assert(data)
	return dynawa.file.save_data(data, self.dir.."my.data")
end

return class

