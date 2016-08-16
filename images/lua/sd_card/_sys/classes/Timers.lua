local class = Class("Timers")

function class:_init()
	self.timer_vectors = {}
end

function class:cancel_all()
	for k,v in pairs(self.timer_vectors) do
		dynawa.timer.cancel(k)
	end
	self.timer_vectors = {}
end

function class:cancel(handle)
	dynawa.timer.cancel(handle)
	self.timer_vectors[handle] = nil
end

function class:timed_event(event)
	if event.delay < 1 then
		event.delay = 1
	end
	event.type = "timed_event"
	assert(event.receiver,"Delayed call has no receiver")
	local handle = assert(dynawa.timer.start(event.delay, event.autorepeat))
	self.timer_vectors[handle] = event
--[[	local n = 0
	for k,v in pairs(self.timer_vectors) do
		n = n + 1
	end
	log(n.." timers active")]]
	return handle
end

function class:dispatch_timed_event(handle)
	local event = self.timer_vectors[handle]
	if not event then
		log("Timer "..tostring(handle).." should fire but its vector is unknown")
	else
		local receiver = event.receiver
		if not event.autorepeat or receiver.__deleted then
			self.timer_vectors[handle] = nil
		end
		if not receiver.__deleted then
			--log("Timed event for "..receiver)
			receiver:handle_event(event)
		end
	end
end

return class

