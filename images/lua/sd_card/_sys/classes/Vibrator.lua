local class = Class("Vibrator", Class.EventSource)


function class:_init()
	Class.EventSource._init(self,"vibrator")
	self:alert_stop()
end

function class:on()
	dynawa.x.vibrator_set(true)
	self.status = true
	self:generate_event{type="vibrator",vibrating=true}
end

function class:off()
	dynawa.x.vibrator_set(false)
	self.status = false
	self:generate_event{type="vibrator",vibrating=false}
end

function class:alert_stop()
	self.alert_status = false
	self.alert_id = nil
	self:off()
end

function class:alert(args) --cycles,tbuzz,tsilence,dbuzz,dsilence,id
	args = args or {}
	local dbuzz = args.dbuzz or 0
	local dsilence = args.dsilence or 0
	local tbuzz = args.tbuzz or 100
	local tsilence = args.tsilence or 900
	local cycles = args.cycles or 3
	self:alert_stop()
	self.alert_status = true
	self.alert_id = args.id or ("vibration"..dynawa.unique_id())
	self:on()
	dynawa.devices.timers:timed_event{delay = tbuzz, receiver = self, buzzing = true, cycles = cycles, tbuzz = tbuzz, tsilence = tsilence, dbuzz = dbuzz, dsilence = dsilence}
end

function class:handle_event_timed_event(event)
	if event.buzzing then
		self:off()
		if event.cycles <= 1 then
			self:alert_stop()
		else
			event.tbuzz = math.max(event.tbuzz + event.dbuzz, 50)
			dynawa.devices.timers:timed_event{delay = event.tsilence, receiver = self, buzzing = false, cycles = event.cycles, tbuzz = event.tbuzz, tsilence = event.tsilence, dbuzz = event.dbuzz, dsilence = event.dsilence}
		end
	else
		assert(event.buzzing == false)
		self:alert{cycles = event.cycles - 1, tbuzz = event.tbuzz, tsilence = math.max(event.tsilence + event.dsilence, 50), dbuzz = event.dbuzz, dsilence = event.dsilence}
	end
end

return class
