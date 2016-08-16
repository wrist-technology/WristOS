app.name = "Button Test App"
app.id = "dynawa.button_test"

function app:start()
	self.window = self:new_window()
	self.window:show_bitmap(dynawa.bitmap.new(160,128))
	self.count = 0
	self:scroll_line("BUTTONS+SCROLLING TEST APP :)")
end

function app:scroll_line(text)
	local line, width, height = dynawa.bitmap.text_line(text)
	local screen = dynawa.bitmap.copy(self.window.bitmap,0,height,160,128)
	local line2 = dynawa.bitmap.combine(dynawa.bitmap.new(160,height,math.random(200),math.random(200),math.random(200)),line,0,0)
	dynawa.bitmap.combine(screen,line2,0,128-height)
	self.window:show_bitmap(screen)
end

function app:switching_to_front()
	self.window:push()
end

function app:handle_event_button(msg)
	self.count = self.count + 1
	self:scroll_line(self.count..": "..msg.button.." "..msg.action)
end

return app
