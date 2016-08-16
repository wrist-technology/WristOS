local class = Class("Buttons", Class.EventSource)

function class:_init()
	Class.EventSource._init(self,"buttons")
	self.raw = Class.EventSource("buttons.raw")
	self.virtual = Class.EventSource("buttons.virtual")
	self.flip_table = {
		[false]={[0]="top","confirm","bottom","switch","cancel"},
		[true]={[0]="bottom","confirm","top","cancel","switch"},
	}
	
	self.raw:register_for_events(self)
	self.matrix = {}
end

function class:update_matrix(event)
	local button = event.button
	local action = event.action
	if action == "button_up" then
		self.matrix[button] = nil
	elseif action == "button_down" then
		self.matrix[button] = dynawa.ticks()
	else
		assert(action == "button_hold")
		self.matrix[button] = 0 - dynawa.ticks()
	end
--[[	for k,v in pairs(self.matrix) do
		log(k..":"..v)
	end
	log("--------")]]
end

function class:handle_event(event)
	local typ = assert(event.type)
	if typ == "button_up" or typ == "button_down" or typ == "button_hold" then
		local event2 = {type = "button", action = typ}
		event2.button = self.flip_table[dynawa.devices.display.flipped][assert(event.button)]
		self:generate_event(event2)
		if typ == "button_up" and event2.button == "switch" then
			if (self.matrix.switch or -1) > 0 then
				self.virtual:generate_event{type = "do_switch"}
			end
		elseif typ == "button_hold" then
			if event2.button == "switch" then
				self.virtual:generate_event{type = "do_superman"}
			elseif event2.button == "cancel" then
				self.virtual:generate_event{type = "do_menu"}
			end
		end
		self:update_matrix(event2)
	else
		log("Event of type "..typ.." unhandled in "..self)
	end
end

return class

