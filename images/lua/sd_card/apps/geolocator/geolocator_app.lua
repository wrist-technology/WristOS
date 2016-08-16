app.name = "Geolocator"
app.id = "dynawa.geolocator"


function app:do_geo_request()
	local georeq = dynawa.app_manager:app_by_id("dynawa.geo_request")
	if not georeq then
		dynawa.popup:error("Geo Request app is not running")
		return nil, "No Geo Request App"
	end
	local request = {command = "geo_request", method = "cached", callback = function(reply,request)
		self:response(reply,request)
	end}
	local stat,err = georeq:make_request(request)
	if not stat then
		dynawa.popup:error("Geolocator cannot work: "..err)
	end
end

function app:menu_item_selected(args)
	local value = args.item.value
	if not value then
		return
	end
	self["menu_action_"..value.jump](self,value)
end

function app:response(response,request)
	--log("Geolocator Geo Response: "..dynawa.file.serialize(response))
	local loc
	if response.gps and ((not response.network) or response.gps.timestamp < 30000 or response.gps.timestamp < response.network.timestamp) then
		loc = response.gps
	elseif response.network then
		loc = response.network
	else
		dynawa.popup:error("Geolocator cannot determine your position. Try again.")
		return
	end
	loc.location.timestamp = dynawa.ticks() - loc.timestamp
	loc.location.accuracy = loc.accuracy
	self.status.location = loc.location
	self:update()
	self:reverse_geoloc_request()
	self:map_request()
end

function app:map_request()
	local loc = assert(self.status.location)
	local zoom = assert(self.status.map_zoom)
	self.status.map = false
	local url = "/maps/api/staticmap?center="..loc.latitude..","..loc.longitude.."&maptype=hybrid&zoom="..zoom.."&size=160x128&sensor=true"
	local http_app = dynawa.app_manager:app_by_id("dynawa.http_request")
	if not http_app then
		dynawa.popup:error("HTTP Request app not available.")
		return
	end
	local request = {timeout = self.timeout, address = "maps.google.com", path = url}
	request.callback = function(response,request)
		self:map_response(response,request)
	end
	local status, err = http_app:make_request(request)
	if not status then
		dynawa.popup:error("Cannot request Google map: "..err)
	end
end

function app:map_response(response)
	--log("Got map")
	if response.status == "200 OK" then
		--log("PNG has "..#(response.body).." bytes")
		local map_bmp = assert(dynawa.bitmap.from_png(response.body),"Cannot parse PNG")
		self.status.map = map_bmp
	else
		self.status.map = "invalid"
	end
	self:update()
end

function app:reverse_geoloc_request()
	local loc = assert(self.status.location)
	local params = "latlng="..loc.latitude..","..loc.longitude.."&sensor=true"
	local request = {timeout = self.timeout, sanitize_text = true, address = "maps.googleapis.com", path = "/maps/api/geocode/json?"..params}
	request.callback = function(response, request)
		self:reverse_geoloc_response(response,request)
	end
	local http_app = dynawa.app_manager:app_by_id("dynawa.http_request")
	if not http_app then
		dynawa.popup:error("HTTP Request app not available!")
		return
	end
	--log("Asking for reverse geoloc (accuracy="..loc.accuracy..")")
	local status,err = http_app:make_request(request)
	if not status then
		dynawa.popup:error("Cannot do reverse geolocation: "..err)
	end
end

function app:reverse_geoloc_response(response)
	--log("Got reverse geoloc: Status = "..tostring(response.status).." ("..tostring(response.error)..")")
	local address = ""
	if response.status == "200 OK" then
		for addr in response.body:gmatch('"formatted_address": "(.-)"') do
			if #addr > #address then
				address = addr
			end
		end
	end
	if address == "" then
		address = "Unable to fetch ("..tostring(response.status)..", "..tostring(response.error)..")"
	end
	--log("Street address: "..address)
	self.status.address = address
	self:update()
end

function app:going_to_sleep()
	return "remember"
end

function app:start()
	self.timeout = 15000
end

function app:switching_to_front()
	if not(self.status and self.status.location) then
		self:reset()
	end
	self:update()
	self.window:push()
end

function app:reset()
	self.status = {map = false, map_zoom = 14, location = false, displaying = "text", address = false}
	self:update()
	self:do_geo_request()
end

function app:update(switch) --switch == boolean
	local win_id = self.status.displaying
	if switch then
		if win_id == "text" then
			win_id = "map"
		else
			win_id = "text"
		end
		self.status.displaying = win_id
	end
	--log("Updating window "..win_id)
	if not self.window then
		self.window = self:new_window()
	end
	self.window:fill()
	if win_id == "text" then
		local loc_txt = "Location unknown."
		if self.status.location then
			local ago_num = math.floor((dynawa.ticks() - self.status.location.timestamp) / 1000 + 0.5)
			local ago_txt = "seconds"
			if ago_num > 120 then
				ago_num = math.floor(ago_num / 60 + .5)
				ago_txt = "minutes"
				if ago_num > 120 then
					ago_num = math.floor(ago_num / 60 + .5)
					ago_txt = "hours"
				end
			end
			loc_txt = string.format("Location: %3.5f (lat), %3.5f (long), accuracy %d m, %s %s ago.", self.status.location.latitude, self.status.location.longitude, math.floor(self.status.location.accuracy + 0.5), ago_num, ago_txt)
		end
		local addr_txt = "Address: Not determined yet."
		if self.status.address then
			addr_txt = "Address: "..self.status.address.."."
		end
		local map_txt = "Map not retrieved yet."
		if self.status.map then
			if self.status.map == "invalid" then
				map_txt = "Couldn't fetch map from Google."
			else
				map_txt = "Press CONFIRM to display map."
			end
		end
		local items = {}
		for i,text in ipairs{loc_txt, addr_txt, map_txt, "Press TOP to force update."} do
			table.insert(items, (dynawa.bitmap.text_lines{text = text, font = "/_sys/fonts/default10.png"}))
		end
		local bmp,w,h = dynawa.bitmap.layout_vertical(items,{spacing = 5})
		self.window:show_bitmap_at(bmp, 0 , 0)
	else --showing map
		local txt
		if not self.status.map then
			txt = "Map (zoom "..self.status.map_zoom..") not yet retrieved. Please wait..."
		end
		if self.status.map == "invalid" then
			txt = "Unable to fetch map from Google."
		end
		if txt then
			local txtbmp = dynawa.bitmap.text_lines{text = txt, font = "/_sys/fonts/default15.png", color = {255,0,0}}
			self.window:show_bitmap_at(txtbmp,0,0)
		else
			self.window:show_bitmap(self.status.map)
			if not self.map_instructions_shown then
				self.map_instructions_shown = true
				dynawa.popup:info("Press CONFIRM to switch back to text info. Press TOP or BOTTOM to zoom in or out.")
			end
		end
	end
end

function app:handle_event_button(event)
	if event.action == "button_down" and event.button == "confirm" then
		self:update(true)
	elseif self.status.displaying == "text" and event.action == "button_down" and event.button == "top" then
			self:reset()
	elseif self.status.displaying == "map" and event.action == "button_down" then
		if event.button == "top" or event.button == "bottom" then
			if not self.status.map then
				dynawa.popup:error("Already fetching map. Please wait.")
			else
				local zoom = self.status.map_zoom
				if event.button == "top" then
					zoom = zoom + 2
				else
					zoom = zoom - 2
				end
				if zoom < 2 then
					dynawa.popup:error("Already at minimum zoom!")
				elseif zoom > 20 then
					dynawa.popup:error("Already at maximum zoom!")
				else
					self.status.map_zoom = zoom
					self:map_request()
					self:update()
				end
			end
		end
	else
		getmetatable(self).handle_event_button(self,event) --Parent's handler
	end
end

function app:switching_to_back()
	self.window:remove_from_stack()
	self.window:_delete()
	self.window = nil
end

