app.name = "Trip Tracker"
app.id = "dynawa.trip_tracker"

function app:start()
	self.prefs = self:load_data() or {units = "metric", default_screen = "basic"}
	self.run_id = false
	self.values = {timestamp = -99999}
	self:gfx_init()
--[[	dynawa.app_manager:after_app_start("dynawa.dyno", function(dyno)
		dyno.events:register_for_events(self, function(ev)
			return (ev.data and ev.data.command == "geo_update" and )
		end)
	end)]]
--	self:handle_event_timed_event()
end

function app:print_at(str,x,y)
	assert(#str > 0)
	local txtbmp = dynawa.bitmap.text_line(str,"/_sys/fonts/default15.png")
	self.window:show_bitmap_at(txtbmp,x,y)
end

function app:megaprint_at(str,x,y)
	for i = 1, #str do
		local chr = str:sub(i,i)
		if chr ~= " " then
			local chrnum = 11
			if chr == "-" then
				chrnum = 10
			elseif (chr >= "0" and chr <= "9") then
				chrnum = assert(tonumber(chr), "Char is '"..chr.."'")
			end
			self.window:show_bitmap_at(self.gfx[chrnum],x+17*(i-1), y)
		end
	end
end

function app:round(num)
	return math.floor(num + 0.5)
end

function app:update_screen()
	self.window:fill()
	local screen = assert(self.screen)
	if screen == "basic" then
		local lines = {}
		for line,name in ipairs({"latitude","longitude","accuracy","altitude","speed","bearing","timestamp"}) do
			local str = name..": "..tostring(self.values[name])
			local txtbmp = dynawa.bitmap.text_line(str,"/_sys/fonts/default10.png")
--			self.window:show_bitmap_at(txtbmp, 0, 11*line - 11)
		end
		local imperial = (self.prefs.units == "imperial")
		local unit = "kph"
		if imperial then
			unit = "mph"
		end
		self:print_at("SPD("..unit..")",0,0)
		local num = self.values.speed
		if not num then
			num = "  ?"
		else
			num = num * 3.6
			if imperial then
				num = num * 0.621371192
			end
			if num > 999 then
				num = 999
			end
			num = string.format("%3d",self:round(num))
		end
		self:megaprint_at(num,0,16)

		unit = "m"
		if imperial then
			unit = "ft"
		end
		self:print_at("ALT("..unit..")",110,0)
		num = self.values.altitude
		if not num then
			num = "    ?"
		else
			if imperial then
				num = num * 3.2808399
			end
			if num > 99999 then
				num = 99999
			elseif num < -9999 then
				num = -9999
			end
			num = string.format("%5d", self:round(num))
		end
		self:megaprint_at(num,76,16)

		self:print_at("BEARING",26,55)
		num = self.values.bearing
		if not num then
			num = "???"
		else
			num = string.format("%03d",self:round(num) % 360)
		end
		self:megaprint_at(num,90,45)
		
		if self.values.latitude then
			self:print_at(string.format("LAT: %3.5f",self.values.latitude),0,75)
		end
		if self.values.longitude then
			self:print_at(string.format("LONG: %3.5f",self.values.longitude),0,90)
		end
		
		if self.values.accuracy then
			local acc = self.values.accuracy
			if imperial then
				acc = self:round(acc * 3.2808399).. " ft"
			else
				acc = self:round(acc).. " m"
			end
			self:print_at("Accuracy: +/- "..acc,0,112)
		end

	elseif screen == "xxbasic" then
		local speed = math.random(140)
		local spd1 = math.floor(speed / 100)
		local spd2 = math.floor(speed / 10) % 10
		local spd3 = speed % 10
		local x,y = 15,15
--		self.window:show_bitmap_at(dynawa.bitmap.new(20,20,255,0,0),10,10)
		self.window:show_bitmap_at(self.gfx[spd3],x+36,y)
		if spd1 + spd2 > 0 then
			self.window:show_bitmap_at(self.gfx[spd2],x+18,y)
		end
		if spd1 > 0 then
			self.window:show_bitmap_at(self.gfx[spd1],x,y)
		end
	else
		error("WTF")
	end
end

function app:handle_response(resp)
	self.values = {timestamp = dynawa.ticks()}
	local vals = self.values
	vals.accuracy = resp.accuracy
	vals.latitude = resp.location.latitude
	vals.longitude = resp.location.longitude
	vals.altitude = resp.altitude
	vals.speed = resp.speed
	vals.bearing = resp.bearing
	self:update_screen()
end

function app:find_geo_app()
	local geo_app = dynawa.app_manager:app_by_id("dynawa.geo_request")
	if not geo_app then
		dynawa.popup:error("Trip Tracker is unable to find a running Geo Request App.")
		return nil
	end
	return geo_app
end

function app:geo_start()
	local geo_app = self:find_geo_app()
	if not geo_app then
		return
	end
	local request = {method = "all", updates = {time = 2000}, id = self.id, callback = function (reply, req)
		log("Got Geo update: "..dynawa.file.serialize(reply))
		self:handle_response(reply)
	end}
	local state,err = geo_app:make_request(request)
	if not state then
		dynawa.popup:error("Cannot start Trip Tracker: "..err)
	end
end

function app:switching_to_front()
	self:update_screen()
	self.window:push()
	self:geo_start()
end

function app:switching_to_back()
	local geo_app = self:find_geo_app()
	if geo_app then
		local request = {updates = "cancel", method = "all", id = self.id}
		geo_app:make_request(request)
	end
	getmetatable(self).switching_to_back(self)
end

function app:going_to_sleep()
	return "remember"
end

function app:gfx_init()
	self.gfx = {}
	local bmp = assert(dynawa.bitmap.from_png_file(self.dir.."gfx.png"))
	for i = 0,11 do
		self.gfx[i] = dynawa.bitmap.copy(bmp,17*i,0,16,24)
	end
	self.window = self:new_window()
	self.screen = self.prefs.default_screen
end

function app:handle_event_do_menu (message)
	local menudef = {
		banner = "Trip Tracker",
		items = {
			{
				text = "Change units", selected = function()
					dynawa.busy()
					if self.prefs.units == "imperial" then
						self.prefs.units = "metric"
					else
						self.prefs.units = "imperial"
					end
					self:save_data(self.prefs)
					dynawa.window_manager:pop():_delete()
					self:update_screen()
					dynawa.popup:info("Units changed to "..self.prefs.units)
				end
			},
		}
	}
	local menuwin = self:new_menuwindow(menudef)
	menuwin:push()
end
