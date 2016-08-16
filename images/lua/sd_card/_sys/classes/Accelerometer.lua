local class = Class("Accelerometer", Class.EventSource)

local gestures = {
	[1] = "wakeup",
	[2] = "sleep",
}

function class:_init()
	Class.EventSource._init(self,"accelerometer")
end

function class:handle_hw_event(event)
	if not dynawa.settings.gestures.enabled then
		return
	end
	self:generate_event{type = "accelerometer", gesture = assert(gestures[event.gesture],"Unknown accelerometer gesture: "..event.type)}
--[
	event.type = nil
	log ("-----Accelerometer HW event:")
	for k,v in pairs(event) do
		log (k.." = "..tostring(v))
	end
	log("-----------")
--]]
end

function class:status()
	local status = assert(dynawa.x.accel_stats(), "No accelerometer status")
	return status
end

return class

