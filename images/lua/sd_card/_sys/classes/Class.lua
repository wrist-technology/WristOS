--Class

local anonymous_class_number = 1

local function tostring_default (self)
	local fn = self.__tostring
	if fn and fn ~= tostring_default and type(fn)=="function" then
		return tostring(fn(self))
	end
	if self:_is_class() then
		return "[class "..self:_name().."]"
	else --instance
		local name = self.name
		if not name and self.id then
			name = "#"..self.id
		end
		local clsname = assert(self:_class():_name())
		if name then
			return "["..name.." ("..clsname..")]"
		else
			return "[instance of "..clsname.."]"
		end
	end
end

local metatable0 = {}

local function add_metamethods(c)
	c.__index = c
	c.__tostring = metatable0.__tostring
	c.__concat = metatable0.__concat
	c.__call = metatable0.__call
end

local function new_class (name, super, bad)
	assert(not bad,"New class creator only accepts 1 argument")
	local c = {}
	if not name then
		name = "AnonymousClass"..anonymous_class_number
		anonymous_class_number = anonymous_class_number + 1
	end
	c.__name = name
	assert(super ~= Class)
	super = super or Class
	assert(super:_is_class(),"Supplied superclass is not a class")
	
	add_metamethods(c)
	setmetatable(c,super)

	assert(not Class[name])
	Class[name] = c
	return c
end

local function new_instance (self, ...)
	local o = {}
	setmetatable(o, self)
	if self._init then
		self._init(o,...)
	end
	return o
end

local invalid_metatable = {
	__index = function (self)
		error("Attempt to access deleted object")
	end,
	__newindex = function (self)
		error("Attempt to modify deleted object")
	end,
}

rawset(_G,"Class", {
    __name = "Class",
})

setmetatable(Class,metatable0)

function Class:_is_class()
	return not not rawget(self,"__index")
end

function Class:_super()
	assert(self:_is_class(),"_super() called on instance")
	if self == Class then
		return Class
	end
	return assert(getmetatable(self))
end

function Class:_class()
	if self:_is_class() then
		return Class
	else
		return assert(getmetatable(self))
	end
end

function Class:_name()
	assert(self:_is_class(),"_name() called on instance")
	return self.__name
end

function Class:_delete()
	assert(not self:_is_class(),"_delete() called on class")
	if self._del then
		self:_del()
	end
	self.__deleted = true
	setmetatable(self,invalid_metatable)
end

local public_classes = {}

metatable0.__tostring = tostring_default
	
metatable0.__concat = function (o1,o2)
	return (tostring(o1)..tostring(o2))
end
	
metatable0.__call = function (self, ...)
	assert(self:_is_class(), "_new() can only be called on a class, not on an instance")
	--log("In _call, self = "..self)
	if self == Class then
		return new_class(...)
	else
		return new_instance(self, ...)
	end
end

add_metamethods(Class)

function Class:handle_event(ev)
	local fname = "handle_event_"..ev.type
	if not self[fname] then
		log("Unhandled event of type '"..tostring(ev.type).."' in "..self)
	else
		return self[fname](self, ev)
	end
end

return Class
