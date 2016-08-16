--BITMAP system init (also printing)

--dynawa.display={flipped=false}
--dynawa.display.size={width=160,height=128}
--dynawa.bitmap.dummy_screen=dynawa.bitmap.new(dynawa.display.size.width, dynawa.display.size.height, 255, 0, 0)
dynawa.fonts = {}

dynawa.bitmap.info = function(bmap)
	assert(type(bmap)=="userdata")
	local peek=dynawa.peek
	local width=peek(2,bmap) + (256*peek(3,bmap))
	local height=peek(4,bmap) + (256*peek(5,bmap))
	
	--log("Bmap bytes:"..res)
	--log("peek(5,bmp) = "..peek(5,bmp))
	return width, height
end

dynawa.bitmap.pixel = function(bmap, x, y)
	local w,h = dynawa.bitmap.info(bmap)
	assert (x >= 0)
	assert (y >= 0)
	assert (x < w)
	assert (y < h)
	local offset = (w * y + x) * 4 + 8
	local peek = dynawa.peek
	--log("offset = "..offset)
	return peek (offset, bmap), peek (offset + 1, bmap), peek (offset + 2, bmap), peek (offset + 3, bmap)
end

--Adds inner border of specified thicknes and color to the bitmap.
dynawa.bitmap.border = function (bitmap, thick, rgb)
	local r,g,b = rgb[1],rgb[2],assert(rgb[3])
	assert(thick > 0)
	local w,h = dynawa.bitmap.info(bitmap)
	local horiz = dynawa.bitmap.new(w,thick,r,g,b)
	dynawa.bitmap.combine(bitmap, horiz, 0, 0)
	dynawa.bitmap.combine(bitmap, horiz, 0, h-thick)
	local vert = dynawa.bitmap.new(thick,h - thick - thick, r,g,b)
	dynawa.bitmap.combine(bitmap, vert, 0, thick)
	dynawa.bitmap.combine(bitmap, vert, w-thick, thick)
	return bitmap
end

dynawa.bitmap.load_font = function (fname)
	local bmap = assert(dynawa.bitmap.from_png_file(fname),"Cannot load font bitmap: "..tostring(fname))
	--log("Parsing font: "..fname)
	local white = dynawa.bitmap.new(20,20,255,255,255)
	local mask = assert(dynawa.bitmap.mask)
	local chars={}
	local widths={}
	local char=32
	local x=0
	local lastx=-1
	local total_width,height = dynawa.bitmap.info(bmap)
	local done = false
	repeat
		x=x+1
		local r,g,b,a = dynawa.bitmap.pixel(bmap,x,0)
		if r+g+b+a == 1020 then
			--log ("Char "..char.." x="..x)
			local width = x-lastx-1
			assert(width >= 1)
			--log("Char dimensions: "..width.."x"..height)
			local char_str = string.char(char)
			local char_bmp = dynawa.bitmap.copy(bmap,lastx+1,0,width,height)
			char_bmp = mask(white,char_bmp,0,0)
			chars[char_str] = char_bmp
			widths[char_str] = width
			lastx = x
			char = char + 1
			if char % 10 == 0 then
				dynawa.busy()
			end
			if char > 128 or x > total_width then
				error("Invalid # of chars in font bitmap")
			end
			r,g,b,a = dynawa.bitmap.pixel(bmap,x,1)
			if r+g+b+a == 1020 then
				done = true
			end
		end
	until done
	--assert (char==128)
	local font = {chars=chars,widths=widths,height=height}
	dynawa.fonts[fname] = font
	return font
end

dynawa.bitmap.text_line = function(line,font,color,size_only)
	assert(type(line)=="string","First parameter is not string")
	--assert(line ~= "", "String is empty")
	if not font then
		font = assert(dynawa.settings.default_font)
	end
	if not dynawa.fonts[font] then
		dynawa.fonts[font] = dynawa.bitmap.load_font(font)
	end
	font = assert(dynawa.fonts[font])
	local x = 0
	local bmaps = {}
	local xs = {}
	for i=1, #line do
		local chr = line:sub(i,i)
		table.insert(bmaps,font.chars[chr])
		table.insert(xs,x)
		x = x + font.widths[chr] + 1
	end
	local width = x - 1
	local height = assert(font.height)
	if size_only then
		return width, height
	end
	local result = dynawa.bitmap.new(width,height,255,255,0,0)
	local combine = dynawa.bitmap.combine
	for i = 1, #line do
		combine(result, bmaps[i], xs [i], 0)
	end
	if color then
		assert(#color == 3,"Color should have 3 numeric elements (r,g,b)")
		if color[1] + color[2] + color[3] ~= 3*255 then
			result = dynawa.bitmap.mask(dynawa.bitmap.new(width,height,color[1],color[2],color[3]),result,0,0)
		end
	end
	return result, width, height
end

function dynawa.bitmap.text_lines(args)
	local words = {}
	args.width = args.width or dynawa.devices.display.size.w - 4
	assert(args.width >= 20, "Width is less than 20")
	local text = assert(args.text, "No text supplied")
	string.gsub(text.." ","(.-) ", function(word)
		if word == "" then
			word = " "
		end
		--log("word:"..word)
		local bitmap, w, h = dynawa.bitmap.text_line(word, args.font, args.color)
		table.insert(words, {text = word, bitmap = bitmap, w = w, h = h})
	end)
	assert(#words > 0)
	local _dummy, space_width, line_height = dynawa.bitmap.text_line(" ",args.font)
	local lines, line = {}, {}
	for i, word in ipairs(words) do
		if not next(line) then
			line = {w=0,h=line_height,w=0,words={}}
		end
		local new_w = line.w + word.w
		if line.w > 0 then
			new_w = new_w + space_width
		end
		if new_w > args.width and (#line.words > 0) then --New line
			table.insert(lines, line)
			line = {words = {word}, h = line_height, w=word.w}
		else
			table.insert(line.words, word)
			line.w = new_w
		end
	end
	if line.w > 0 then
		table.insert(lines, line)
	end
	--log("Lines = "..#lines)
	local maxwidth = 0
	local given_w = args.width
	for i, line in ipairs(lines) do
		if line.w > given_w then
			assert(#line.words == 1,"Line is too wide but it has more than 1 word")
			local word = line.words[1]
			local text = assert(word.text)
			assert(#text >= 5)
			local center = math.floor((#text + 1) / 2)
			local left = center - 1
			local right = center + 1
			local flip = true
			repeat
				word.text = text:sub(1,left).."..."..text:sub(right)
				if flip then
					right = right + 1
					assert(right < #text)
				else
					left = left - 1
					assert(left > 1)
				end
				flip = not flip
				word.w, word.h = dynawa.bitmap.text_line(word.text, args.font, nil, true)
			until word.w <= given_w
			word.bitmap = dynawa.bitmap.text_line(word.text, args.font, args.color)
			line.w = word.w
		end
		if line.w > maxwidth then
			maxwidth = line.w
		end
	end
	local width = args.width
	if args.autoshrink then
		assert(maxwidth <= width)
		width = maxwidth
	end
	local height = #lines * line_height
	local bitmap = dynawa.bitmap.new(width, height, 255,0,0,0)
	local b_combine = dynawa.bitmap.combine
	for i, line in ipairs(lines) do
		local x = (i-1) * line_height
		local y = 0
		if args.center then
			y = math.floor((width - line.w)/2)
		end
		for j, word in ipairs(line.words) do
			b_combine(bitmap, word.bitmap, y, x)
			y = y + word.w + space_width
		end
	end
	return bitmap, width, height
end

function dynawa.bitmap.layout_vertical(items0, args)
	args = args or {}
	args.spacing = args.spacing or 0
	args.border = args.border or 0
	args.bgcolor = args.bgcolor or {0,0,0,0} --No bgcolor -> Transparent background
	local items = {}
	local sizew, sizeh = 0, 0
	for i, bitmap in ipairs(items0) do
		local w,h = dynawa.bitmap.info(bitmap)
		if sizew < w then
			sizew = w
		end
		table.insert(items, {bitmap = bitmap, y = sizeh, w=w})
		sizeh = sizeh + h
		if i < #items0 then
			sizeh = sizeh + args.spacing
		end
	end
	local border = args.border
	local border2 = border + border
	local bmap
	bmap = dynawa.bitmap.new (sizew + border2, sizeh + border2, unpack(args.bgcolor))
	for i, item in ipairs(items) do
		local x = 0
		if args.align == "center" then
			x = math.floor((sizew - item.w)/2)
		elseif args.align == "right" then
			x = sizew - item.w
		end
		dynawa.bitmap.combine (bmap, item.bitmap, border + x, border + item.y)
	end
	return bmap, sizew + border2, sizeh + border2
end

function dynawa.bitmap.layout_horizontal(items0, args)
	args = args or {}
	args.spacing = args.spacing or 0
	args.border = args.border or 0
	args.bgcolor = args.bgcolor or {0,0,0,0} --No bgcolor -> Transparent background
	local items = {}
	local sizew, sizeh = 0, 0
	for i, bitmap in ipairs(items0) do
		local w,h = dynawa.bitmap.info(bitmap)
		if sizeh < h then
			sizeh = h
		end
		table.insert(items, {bitmap = bitmap, x = sizew, h=h})
		sizew = sizew + w
		if i < #items0 then
			sizew = sizew + args.spacing
		end
	end
	local border = args.border
	local border2 = border + border
	local bmap
	bmap = dynawa.bitmap.new (sizew + border2, sizeh + border2, unpack(args.bgcolor))
	for i, item in ipairs(items) do
		local y = 0
		if args.align == "center" then
			y = math.floor((sizeh - item.h)/2)
		elseif args.align == "bottom" then
			y = sizeh - item.h
		end
		dynawa.bitmap.combine (bmap, item.bitmap, border + item.x, border + y)
	end
	return bmap, sizew + border2, sizeh + border2
end

local screen = dynawa.bitmap.new(160,128,0,0,99)
dynawa.bitmap.combine(screen,dynawa.bitmap.text_lines{width = 156, text=
		"WristOS "..dynawa.version.wristOS.."; settings rev. "..dynawa.version.settings_revision.."; main.bin: ???"
		},2,2)
dynawa.bitmap.show(screen)
dynawa.busy()

