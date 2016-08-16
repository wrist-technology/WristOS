app.name = "DynaBlob"
app.id = "dynawa.dynablob"

--[[
	States:
	normal
	trippy
	happy
	unhappy
	sleeping
	sick
]]

function app:start()
	self.window = self:new_window()
	self.window:fill()
	self:init_indices()
	self:init_skelets()
	self.background = assert(dynawa.bitmap.from_png_file(self.dir.."background.png"))
	self.states = {"normal","trippy","sleeping"}
end

function app:switching_to_front()
	self.window:push()
	self.run_id = dynawa.unique_id()
	self:re_color()
	self.blob = {count = 0, skelet = self.skelets[1], state = "normal"}
	self:animate()
end

function app:handle_event_button(event)
	if event.action == "button_down" and event.button == "bottom" then
		local sk = table.remove(self.skelets)
		table.insert(self.skelets,1,sk)
		self.blob = {count = 0, skelet = self.skelets[1], state = self.states[1]}
	end
	if event.action == "button_down" and event.button == "top" then
		local sk = table.remove(self.states)
		table.insert(self.states,1,sk)
		self.blob = {count = 0, skelet = self.skelets[1], state = self.states[1]}
	end
	getmetatable(self).handle_event_button(self,event) --Parent's handler
end

function app:re_color()
	local color = {math.random(200)+55, math.random(200)+55, math.random(200)+55}
	--color = {255,0,255}
	local full = dynawa.bitmap.new(160,128,color[1],color[2],color[3])
	for id, sprite in pairs(self.sprites) do
		if id:match("^blob_.*") then
			sprite.bitmap = dynawa.bitmap.mask(full, sprite.bitmap,0,0)
		end
	end
end

local random_eyes = {"eye_top", "eye_lb", "eye_closed", "eye_rt", "eye_right", "eye_center"}
local trippy_mouths = {"mouth_wave","mouth_closed","mouth_open","mouth_teeth"}

function app:animate()
	local blob = self.blob
	self.window:show_bitmap_at(self.background,0,0)
	local state = assert(blob.state)
	local count = assert(blob.count)
	local eyel,eyer
	if state == "normal" then
		eyel = "eye_rb"
		if math.random() > 0.8 then
			eyel = random_eyes[math.random(#random_eyes)]
		end
		eyer = eyel
	elseif state == "trippy" then
		if math.random() < 0.2 then
			eyel = "eye_x"
			eyer = "eye_x"
		else
			eyel = "eye_trippy"..(count % 2 + 1)
			eyer = eyel
		end
	elseif state == "sleeping" then
		eyel = "eye_closed"
		eyer = "eye_closed"
	else
		error("WTF")
	end
	blob.eye_l = assert(eyel)
	blob.eye_r = assert(eyer)
	
	local mouth
	if state == "normal" then
		if math.random() > 0.8 then
			mouth = "mouth_closed"
		else
			mouth = "mouth_smile"
		end
	elseif state == "trippy" then
		mouth = trippy_mouths[math.random(#trippy_mouths)]
	elseif state == "sleeping" then
		if count % 20 < 7 then
			mouth = "mouth_open"
		else
			mouth = "mouth_closed"
		end
	else
		error("WTF")
	end
	
	blob.mouth = assert(mouth)
	
	local wait_ms = blob.skelet.animate(self,blob)
	assert(wait_ms >= 100)
	blob.count = blob.count + 1
	--local eyes = {"eye_rb","eye_lb","eye_top","eye_rt","eye_closed","eye_right","eye_center","eye_squint","eye_blood1","eye_blood2","eye_smaller"}
	dynawa.devices.timers:timed_event{delay = wait_ms, receiver = self, run_id = self.run_id}
end

function app:handle_event_timed_event(event)
	if self.run_id ~= event.run_id then
		return
	end
	if not self.window.in_front then
		self.run_id = nil
		return
	end
	self:animate(0)
end

function app:init_skelets()
	self.skelets = {}
	self.skelets[1] = { --embryo
		animate = function(self,blob)
			local count = blob.count
			local radius = 2
			local period = 12
			if blob.state == "sleeping" then
				radius = 1
				period = 30
			elseif blob.state == "trippy" then
				radius = 4
				period = 8
			end
			self:put_sprite("blob_1", self:xy_add(0,10,self:anim_circle(count,period,radius)))
			local facex,facey = self:xy_add(0,20,self:anim_circle(count+4,period,radius))
			self:put_sprite("blob_5", facex, facey)
			self:put_sprite(blob.eye_l,facex - 5, facey + 5)
			self:put_sprite(blob.eye_r,facex + 5, facey + 5)
			self:put_sprite(blob.mouth,facex,facey-5)
			return 100
		end
	}
	self.skelets[2] = { --small
		animate = function(self,blob)
			local count = blob.count
			local diff = count % 20
			if diff > 10 then
				diff = 20 - diff
			end
			diff = diff - 5
			local jmp = count % 3
			self:put_sprite("blob_4",0,8)
			self:put_sprite("blob_3",diff, 22 + jmp)
			self:put_sprite("blob_3",diff * 2, 37)
			self:put_sprite("blob_2",diff,48)
			local eyediff = 10
			local eye = "eye_rb"
			self:put_sprite(blob.eye_r, diff + eyediff,48)
			self:put_sprite(blob.eye_l,diff - eyediff,48)
			self:put_sprite(blob.mouth,diff * 0.5,28)
			return 100
		end
	}
	self.skelets[3] = {--"worm"
		animate = function(self,blob)
			local count = blob.count
			local radius = 3
			local period = 12
			if blob.state == "sleeping" then
				radius = 2
				period = 30
			elseif blob.state == "trippy" then
				radius = 6
				period = 8
			end
			self:put_sprite("blob_5", self:xy_add(-45,5,self:anim_circle(count,period,radius)))
			self:put_sprite("blob_1", self:xy_add(-30,5,self:anim_circle(count+1,period,radius)))
			self:put_sprite("blob_1", self:xy_add(-10,7,self:anim_circle(count+2,period,radius)))
			self:put_sprite("blob_2", self:xy_add(10,10,self:anim_circle(count+3,period,radius)))
			self:put_sprite("blob_3", self:xy_add(35,20,self:anim_circle(count+4,period,radius)))
			local facex,facey = self:xy_add(40,20,self:anim_circle(count+4,period,radius))
			if blob.state == "sleeping" then
				facey = facey + 5 - (count % 20) / 2
			end
			self:put_sprite(blob.eye_l,facex - 12, facey + 8)
			self:put_sprite(blob.eye_r,facex + 12, facey + 8)
			self:put_sprite(blob.mouth,facex,facey-5)
			return 100
		end
	}
end

function app:xy_add(x,y,xx,yy)
	return x+xx,y+yy
end

function app:anim_circle(count,period,sizex,sizey)
	sizey = sizey or sizex
	local angle = 2 * 3.1415926536 * (count % period) / period
	return math.sin(angle)*sizex, math.cos(angle)*sizey
end


function app:put_sprite(sprid, cx, cy, window)
	window = window or self.window
	local sprite = self.sprites[sprid]
	if not sprite then
		error("Uknown sprite id: "..sprid)
	end
	local dx = 80
	window:show_bitmap_at(sprite.bitmap, math.floor(0.5 + cx - sprite.half_size.w + dx), math.floor(128.5 - cy - sprite.half_size.h))
end

function app:init_indices()
	local parts = assert(dynawa.bitmap.from_png_file(self.dir.."parts.png"))
	local indices = {
		["testmark"] = {0,0,4,4},
		["blob_1"] = {9,63,46,94},
		["blob_2"] = {6,100,58,149},
		["blob_3"] = {68,66,141,114},
		["blob_4"] = {75,127,145,150},
		["blob_5"] = {151,74,174,98},
		["blob_6"] = {162,109,197,155},
		["blob_7"] = {205,58,329,164},
		["eye_rb"] = {15,0,25,11},
		["eye_top"] = {25,0,35,11},
		["eye_lb"] = {35,0,45,11},
		["eye_closed"] = {45,3,54,6},
		["eye_x"] = {54,0,61,7},
		["eye_rt"] = {72,0,82,11},
		["eye_right"] = {82,0,92,11},
		["eye_center"] = {92,0,102,11},
		["eye_squint"] = {102,0,112,11},
		["eye_blood1"] = {112,0,122,11},
		["eye_blood2"] = {122,0,132,11},
		["eye_smaller"] = {132,0,142,9},
		["eye_trippy1"] = {142,0,152,11},
		["eye_trippy2"] = {152,0,162,11},
		["mouth_teeth"] = {0,19,14,29},
		["mouth_smile"] = {15,23,29,27},
		["mouth_bigsmile"] = {30,19,44,29},
		["mouth_frown"] = {45,22,59,27},
		["mouth_open"] = {62,19,72,29},
		["mouth_closed"] = {77,23,87,26},
		["mouth_wave"] = {88,22,98,27},
	}
	self.sprites = {}
	for id, ind in pairs(indices) do
		local x,y = ind[1],ind[2]
		local w,h = ind[3] - x, ind[4] - y
		assert(w>0)
		assert(h>0)
		self.sprites[id] = {bitmap = dynawa.bitmap.copy(parts,x,y,w,h),size = {w=w,h=h}, half_size = {w=math.floor(w/2), h=math.floor(h/2)}}
	end
end

return app
