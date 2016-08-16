app.name = "Geo Request Service"
app.id = "dynawa.geo_request"

function app:start()
	self.requests = {}
	dynawa.app_manager:after_app_start("dynawa.dyno", function(dyno)
		dyno.events:register_for_events(self, function(ev)
			return (ev.data and (ev.data.command == "geo_response" or ev.data.command == "geo_update"))
		end)
	end)
--	self:handle_event_timed_event()
end

function app:cleanup()
	local count = 0
	local time = dynawa.ticks()
	for id,request in pairs(self.requests) do
		if time - request.timestamp > 60000 * 60 then --1 hour limit
			self.requests[id] = nil
			count = count + 1
		end
	end
	if count > 0 then
		log("Geo requester - Cleaned up requests = "..count)
	end
end

function app:make_request(request)
	self:cleanup()
	request.command = "geo_request"
	--assert(request.method, "Geo request has no method")
	request.id = request.id or dynawa.unique_id()
	request.timestamp = dynawa.ticks() 
	local dyno = dynawa.app_manager:app_by_id("dynawa.dyno")
	if not dyno then
		--Don't show popup for unsuccesful 'cancel'!
		if request.updates ~= "cancel" then
			dynawa.popup:error("Dyno is not running, Geo Request Service cannot control the phone")
		end
		return false,"No Dyno"
	end
	local act = dyno:try_activity(self.last_bad_bdaddr)
	if not act then
		log("Geo Requester: Dyno doesn't have any activity whose status is 'connected'.") --#todo
		return nil, "Dyno not connected"
	end 
	request.bdaddr = act.bdaddr
	if request.updates == "cancel" then
		self.requests[request.id] = nil
	else
		self.requests[request.id] = request
	end
	local callback = request.callback
	request.callback = nil
	log("Sending Geo request "..request.id)
	local stat,err = dyno:bdaddr_send_data(act.bdaddr,request)
	request.activity = act
	request.callback = callback
	if not stat then
		log("Cannot send Geo request: "..err)
		return nil, "Cannot send Geo request: "..err
	end
	return true
end

function app:find_request(response)
	local id = assert(response.id, "Missing id in response")
	local request = self.requests[id]
	if not request then
		log("Unknown geo response with id="..id.." (cancelling)")
		local dyno = assert(dynawa.app_manager:app_by_id("dynawa.dyno"))
		dyno:bdaddr_send_data(assert(response.bdaddr),{id = id, updates = "cancel"})
		return nil
	end
	return request
end

function app:handle_event_dyno_data_from_phone(ev)
	self:handle_response(assert(ev.data))
end

--[[function app:handle_event_timed_event(ev)
	self:send_request()
	dynawa.devices.timers:timed_event{delay = 30000, receiver = self}
end]]

function app:handle_response(response)
	local request = self:find_request(response)
	--request can be nil if we cancelled geo updates after the geo update was already buffered.
	if request then
		if request.method == "cached" then --It was one-time request. Remove it from requests.
			self.requests[assert(request.id)] = nil
		end
		if response.error then
			--Next time, try different Geo server if possible
			self.last_bad_bdaddr = request.bdaddr
		end
		request.callback(response,request)
	end
end
