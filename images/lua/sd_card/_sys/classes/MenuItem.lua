local class = Class("MenuItem")

function class:_init(desc)
	--#todo text-only so far
	self.text = desc.text
	self.textcolor = desc.textcolor
	self.id = dynawa.unique_id()
	if desc.selected then
		assert(type(desc.selected) == "function", "'selected' attribute is not a function")
		self.selected = desc.selected
	end
	if desc.render then
		assert(type(desc.render) == "function", "'render' attribute is not a function")
		self.render = desc.render
	end
	if desc.value then
		self.value = desc.value
	end
	if not self.textcolor then
		if not(desc.selected or desc.value) then
			self.textcolor = {128,128,128}
		end
	end
end

function class:render(args)
	local bitmap = dynawa.bitmap.text_lines{text=self.text, font = nil, width = assert(args.max_size.w), color = self.textcolor}
	return bitmap
end

function class:selected(args)
	local menu = assert(args.menu)
	return menu:item_selected(args)
end

return class

