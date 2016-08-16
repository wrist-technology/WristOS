require("dynawa")

local clock
local gfx = {}
local dot_size = {w=25,h=17}

local function random_color()
	return {math.random(200)+55, math.random(200)+55, math.random(200)+55}
end

local function convert_coords(x,y)
	local xx = x * 27
	local yy = y * 19
	if y > 2 then
		yy = yy + 16
	end
	return xx,yy
end

local function display(bitmap, x, y)
	assert(bitmap)
	assert(x)
	assert(y)
	dynawa.message.send{type="display_bitmap", bitmap = bitmap, at={x,y}}
end

local function change_dot(x, y, gf, color)
	local recolor = dynawa.bitmap.new(dot_size.w,dot_size.h,unpack(color))
	local fg = dynawa.bitmap.mask(recolor, gfx[gf], 0, 0)
	local dot = dynawa.bitmap.combine(gfx.black, fg, 0, 0, true)
	display(dot, convert_coords(x,y))
end

local function text(time)
	local style = my.globals.prefs.style
	local font = "/_sys/fonts/default10.png"
	local ticks = dynawa.bitmap.text_line(tostring(time.raw),font,{0,0,0})
	local mem, w, h = dynawa.bitmap.text_line(collectgarbage("count") * 1024 .." "..time.wday,font,{0,0,0})
	local bgcolor
	if style == "red" then
		bgcolor = {255,0,0}
	elseif style == "white" then
		bgcolor = {255,255,255}
	elseif style == "blue/green" then
		bgcolor = {0,255,0}
		if time.raw % 2 == 0 then
			bgcolor = {0,0,255}
		end
	else
		bgcolor = {200,200,200}
	end
	local bg = dynawa.bitmap.new (160, h, unpack(bgcolor))
	dynawa.bitmap.combine(bg, ticks, 1, 1)
	dynawa.bitmap.combine(bg, mem, 159 - w, 1)
	local y = math.floor(64 - h / 2)
	display(bg, 0, y)
end

local function to_bin (num)
	local result = {}
	for i=5,0,-1 do
		result[i] = num % 2
		num = math.floor(num / 2)
	end
	return result
end

local function update_dots(time, status)
	local style = my.globals.prefs.style
	local new = {}
	local clk = clock.state
	new[0] = to_bin(time.hour)
	new[1] = to_bin(time.min)
	new[2] = to_bin(time.sec)
	new[3] = to_bin(time.day)
	new[4] = to_bin(time.month)
	new[5] = to_bin(math.min(time.year % 100, 63))
	for i = 0, 5 do
		for j = 0, 5 do
			if clk[i][j].gfx ~= new[i][j] then
				local dot = clk[i][j]
				dot.gfx = new[i][j]
				if status ~= "first" then
					if style ~= "blue/green" and style ~= "white" then
						if style == "red" then
							dot.color = {255,0,0}
						else
							dot.color = random_color()
						end
					end
				end
				change_dot(j,i,dot.gfx,dot.color)
			end
		end
	end
	local i = math.random(6) - 1
	local j = math.random(6) - 1
	local color
	if style ~= "blue/green" and style ~= "white" then
		if style == "red" then
			local cl = math.random(40)
			color = {math.random(100)+155,cl,cl}
		else
			color = random_color()
		end
		change_dot(j,i,clk[i][j].gfx,color)
		clk[i][j].color = color
	end
end

local function tick(message)
	if not my.app.in_front or message.run_id ~= clock.run_id then
		return
	end
	local time_raw, msec = dynawa.time.get()
	local time = os.date("*t", time_raw)
	time.raw = time_raw
	update_dots(time,message.status)
	text(time)
	local sec, msec = dynawa.time.get()
	local when = 1100 - msec
	dynawa.delayed_callback{time = when, callback = tick, run_id = message.run_id}
end

local function start()
	local style = my.globals.prefs.style
	clock = {state={}}
	dynawa.message.send{type="display_bitmap", bitmap = dynawa.bitmap.new(160,128,0,0,0)}
	for i = 0, 5 do
		clock.state[i] = {}
		for j = 0, 5 do
			local color
			if style == "red" then
				color = {150,0,0}
			elseif style == "white" then
				color = {255,255,255}
			elseif style == "blue/green" then
				color = {0,0,255}
				if i >= 3 then
					color = {0,255,0}
				end
			else
				color = random_color()
			end
			clock.state[i][j] = {gfx = "empty", color = color}
			--change_dot(i,j,"empty",color)
		end
	end
	clock.run_id = dynawa.unique_id()
	tick{run_id = clock.run_id, status = "first"}
end

local function to_front()
	start()
end

local function to_back()
	clock = {}
end

local function overview()
	if not my.globals.logo then
		my.globals.logo = assert(dynawa.bitmap.from_png_file(my.dir.."logo.png"))
	end
	return my.globals.logo
end

local function gfx_init()
	local bmap = assert(dynawa.bitmap.from_png_file(my.dir.."gfx.png"))
	local b_copy = dynawa.bitmap.copy
	gfx[0] = b_copy(bmap,0,0,dot_size.w,dot_size.h)
	gfx[1] = b_copy(bmap,25,0,dot_size.w,dot_size.h)
	gfx.empty = b_copy(bmap,50,0,dot_size.w,dot_size.h)
	gfx.black = dynawa.bitmap.new(dot_size.w,dot_size.h,0,0,0)
end

my.app.name = "Bynari Clock 2"
gfx_init()
dynawa.message.receive {message="you_are_now_in_front", callback=to_front}
dynawa.message.receive {message="you_are_now_in_back", callback=to_back}
dynawa.message.receive {message="your_overview", callback=overview}
my.globals.prefs = dynawa.file.load_data() or {style = "default"}
dynawa.message.send{type="display_bitmap", bitmap = dynawa.bitmap.dummy_screen}
dofile(my.dir.."bynari_prefs.lua")

