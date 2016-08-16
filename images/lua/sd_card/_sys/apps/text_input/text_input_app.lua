app.name = "Text input"
app.id = "dynawa.text_input"

function app:switching_to_front()
	self:text_input{text = "Debug", callback = function(args)
		log("Text entry: '"..args.text.."' ("..#args.text.." chars)")
	end
	}
end

function app:switching_to_back()
	self.window:pop():_delete()
	self.window = nil
	self.state = nil
	self.menus = nil
end

function app:text_input(args)
	assert(not self.state, "Already has state when called")
	--#todo charset (email, phone#)
	self:create_menus()
	self.state = {callback = assert(args.callback)}
	self.state.before = args.text or ""
	self.state.after = ""
	self.window = self:new_window()
	self.window:push()
	self.window:fill{0,0,0}
	self:update_text()
	self:update_wheel()
end

function app:update_wheel()
	local blank = dynawa.bitmap.new(self.width,self.window.size.h,0,0,50)
	self.window:show_bitmap_at(blank,self.horizontal,0)
	local font_size = 15
	local font = "/_sys/fonts/default15.png"
	local top = self.vertical
	local offset = 0
	while top > font_size do
		top = top - font_size
		offset = offset - 1
	end
	local offset2 = 0
	local bottom = self.vertical
	while bottom + font_size + font_size < self.window.size.h do
		bottom = bottom + font_size
		offset2 = offset2 + 1
	end
	local symbols = {}
	local menu = self.menus[self.menus.active]
	for off = offset, offset2 do
		local char = menu[self:normalize(menu.index + off,menu)]
		local char_bmp,w,h = dynawa.bitmap.text_line(char, font)
		self.window:show_bitmap_at(char_bmp, self.horizontal + math.floor((self.width - w)/2), top)
		top = top + font_size
	end
end

function app:normalize(val, array)
	return (val - 1)%(#array) + 1
end

function app:update_text()
	local font = "/_sys/fonts/default15.png"
	local font_size = 15
	local c1,c2,c3 = 150,0,0
	self.window:show_bitmap_at(dynawa.bitmap.new(self.horizontal,font_size,c1,c2,c3),0,self.vertical)
	self.window:show_bitmap_at(dynawa.bitmap.new(self.window.size.w - self.horizontal - self.width,font_size,c1,c2,c3),self.horizontal + self.width,self.vertical)
	if self.state.before ~= "" then
		local text_bmp,w,h = dynawa.bitmap.text_line(self.state.before, font)
		self.window:show_bitmap_at(text_bmp, self.horizontal - w, self.vertical)
	end
	if self.state.after ~= "" then
		local text_bmp,w,h = dynawa.bitmap.text_line(self.state.after, font)
		self.window:show_bitmap_at(text_bmp, self.horizontal + self.width, self.vertical)
	end
end

function app:start()
	self.horizontal = 100
	self.vertical = 50
	self.width = 12
end

function app:create_menus()
	local menus = {
--		{"Accept text","Delete char","Move <<","Move >>","DELETE ALL"},
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ ",
		"abcdefghijklmnopqrstuvwxyz ",
		"0123456789 ",
		" .,@?!+-*/:;()[]{}<=>#$%^&_|\\'~",
	}
	self.menus = {}
	for menu_n, menu0 in ipairs(menus) do
		dynawa.busy()
		local menu = {}
		for item_n = 1, #menu0 do
			local item
			item = menu0:sub(item_n, item_n)
			table.insert(menu, item)
		end
		menu.index = 1
		table.insert(self.menus, menu)
	end
	self.menus.active = 1
end

function app:handle_event_button(msg) --top bottom confirm / button_hold button_up button_down
	--log(msg.button.." "..msg.action..", mem="..collectgarbage("count") * 1024)
	if msg.button == "cancel" and msg.action == "button_up" then --Switch to next wheel
		self.menus.active = self:normalize(self.menus.active + 1,self.menus)
		self:update_wheel()
		return
	end
	if msg.button == "top" or msg.button == "bottom" then
		local direction = 1
		if msg.button == "top" then
			direction = -1
		end
		if msg.action == "button_down" then
			local menu = self.menus[self.menus.active]
			menu.index = self:normalize(menu.index + direction, menu)
			self:update_wheel()
			return
		elseif msg.action == "button_hold" then
			if self.state.scrolling then --Already scrolling
				self.state.scrolling = direction
			else
				self.state.scrolling = direction
				self:scroll_step()
			end
			return
		elseif msg.action == "button_up" then
			self.state.scrolling = nil
			return
		end
	end
	if msg.button == "confirm" and msg.action == "button_down" then
		local menu = self.menus[self.menus.active]
		self.state.before = self.state.before .. menu[menu.index]
		self:update_text()
		return
	end
end

function app:scroll_step(msg)
	if not self.state then
		return
	end
	local direction = self.state.scrolling
	if not direction then
		return
	end
	local menu = self.menus[self.menus.active]
	menu.index = self:normalize(menu.index + direction, menu)
	self:update_wheel()
	dynawa.devices.timers:timed_event{delay = 50, receiver = self, subtype = "scrolling"}
end

function app:handle_event_timed_event(msg)
	assert(msg.subtype == "scrolling")
	self:scroll_step()
end

function app:handle_event_do_menu (message)
	local menudef = {
		banner = "Text entry ("..(#self.state.before + #self.state.after).." characters):",
		items = {
			{
				text = "Move left <", value = "move_left"
			},
			{
				text = "Move right >", value = "move_right"
			},
			{
				text = "Delete character", value = "delete"
			},
			{
				text = "Accept entered text and return", value = "accept"
			},
			{
				text = "Move to the beginning <<", value = "jump_left"
			},
			{
				text = "Move to the end >>", value = "jump_right"
			},
			{
				text = "Clear all text", value = "delete_all"
			},
		},
	}
	local menuwin = self:new_menuwindow(menudef)
	menuwin:push()
end

function app:close_menu()
	local win = dynawa.window_manager:pop()
	assert(win.app == self)
	assert(win.menu)
	win:_delete()
end

function app:menu_item_selected(args)
	local command = assert(args.item.value)
	local state = self.state
	log("menu command="..command)
	if command == "move_left" then
		if state.before ~= "" then
			state.after = state.before:sub(-1) .. state.after
			state.before = state.before:sub(1,-2)
			self:update_text()
		end
		self:close_menu()
	elseif command == "move_right" then
		if state.after ~= "" then
			state.before = state.before .. state.after:sub(1,1)
			state.after = state.after:sub(2)
			self:update_text()
		end
		self:close_menu()
	elseif command == "delete" then
		if state.before ~= "" then
			state.before = state.before:sub(1,-2)
			self:update_text()
		end
		self:close_menu()
	elseif command == "accept" then
		local callback = assert(self.state.callback)
		local args = {text = self.state.before .. self.state.after}
		args.before, args.after = self.state.before, self.state.after
		self:close_menu()
		self:switching_to_back()
		callback(args)
	elseif command == "jump_left" then
		state.after = state.before .. state.after
		state.before = ""
		self:update_text()
		self:close_menu()
	elseif command == "jump_right" then
		state.before = state.before .. state.after
		state.after = ""
		self:update_text()
		self:close_menu()
	elseif command == "delete_all" then
		state.before, state.after = "", ""
		self:update_text()
		self:close_menu()
	end
end

return app
