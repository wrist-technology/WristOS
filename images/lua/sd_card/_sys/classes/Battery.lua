local class = Class("Battery", Class.EventSource)

local function log_file(txt)
	if true then
		return
	end
	local time = os.date("*t")
	local timetxt = string.format("%02d:%02d:%02d", time.hour, time.min, time.sec)
	local fd = assert(io.open("/battery_log.txt","a"))
	assert(fd:write(timetxt.." "..txt.."\n"))
	fd:close()
end

function class:_init()
	Class.EventSource._init(self,"battery")
	self.last_critical_status = -999999
	self.last_status = false
	self.pct0 = 3400
	self.pct100 = 4150
	self.critical_voltage = 3500
	dynawa.devices.timers:timed_event{delay = 10000, receiver = self}
	log_file("REBOOT")
end

function class:handle_hw_event(event)
	event.type = nil
	log ("-----Battery HW event:")
	for k,v in pairs(event) do
		log (k.." = "..tostring(v))
	end
	log("-----------")
end

function class:voltage_to_percent(v)
	local pct = (v - self.pct0) / (self.pct100 - self.pct0) * 100
	pct = math.floor(pct + 0.5)
	if pct < 0 then
		return 0
	end
	if pct > 100 then
		return 100
	end
	return pct
end

function class:status()
	local status = {timestamp = os.time()}
	local stat = assert(dynawa.x.battery_stats())
	status.voltage = assert(stat.voltage)
	status.current = assert(stat.current)
	status.percentage = self:voltage_to_percent(status.voltage)
	if stat.state == 1 or stat.voltage == 0 then
		--When no battery is present (voltage == 0), status is set to "charging"
		--#todo In the production version, this should be changed to display nasty warning about battery not working.
		status.charging = true
	end
	if status.voltage <= self.critical_voltage and not status.charging then
		status.critical = true
	end
	local logtxt = "Voltage = "..status.voltage .. " ("..status.percentage.."%) "..status.current.." mA"
	log(logtxt)
	log_file(logtxt)
	return status
end

function class:broadcast_update(event) --Broadcast the change
	event.type = "battery_status"
	self:generate_event(event)
end

function class:handle_event_timed_event(event)
	--if true then return end
	local status = self:status()
	if not self.last_status or (self.last_status.voltage ~= status.voltage or self.last_status.charging ~= status.charging) then
		self:broadcast_update(status)
		self.last_status = {timestamp = status.timestamp, voltage = status.voltage, percentage = status.percentage, charging = status.charging}
	end
	if status.critical then
		local ts = dynawa.ticks()
		--Only 1 alert in 10 minutes to handle voltage fluctuations around critical level
		if ts - self.last_critical_status > 60 * 1000 * 10 then
			dynawa.popup:error("Only "..status.percentage.."% of battery charge remaining. Please recharge your TCH1.")
			dynawa.devices.vibrator:alert{cycles = 1, tbuzz = 1000}
		else
			--The voltage is critical but it's been less than 10 minutes since the last critical popup.
			--Don't display alert but reset "last_critical_status" counter (below),
			--effectively disabling the possibility of alert until battery is charged again AND at least 10 minutes pass.
		end
		self.last_critical_status = ts
	end
	--#todo Dynamic delay?
	local delay = 60000
	dynawa.devices.timers:timed_event{delay = delay, receiver = self}
end

return class
