#!/usr/bin/env lua

local nothing = function() end
log = print
dynawa={dir = {sys = "../"},debug={}}
dynawa.busy = nothing
local classes = {}
dynawa.debug.pc_classinfoXXXXXXXXXXXXXXXX = function(cls)
	if cls ~= Class then
		table.insert(classes,cls)
	end
end
dofile("init.lua")

local add = print
add("Class report @ "..os.date())
for i, class in ipairs(classes) do
	add("----------------------------------")
	local mro = {}
	for i,cls in ipairs(class.__mro) do
		table.insert(mro, cls.__name)
	end
	add("----- Class: "..class:_name())
	add("MRO: "..table.concat(mro,", "))
	add("Properties:")
	local impls = {}
	for i,cls in ipairs(class.__mro) do
		for k, v in pairs(cls) do
			if not (impls[k] or k:match("^__")) then
				impls[k] = cls
			end
		end
	end
	local impls2 = {}
	for k,v in pairs(impls) do
		table.insert(impls2, {key=k,class=v})
	end
	table.sort(impls2,function(a,b)
		--[[if a.class ~= b.class then
			if a.class == class then
				return true
			end
			if b.class == class then
				return false
			end
		end]]
		return (a.key < b.key)
	end)
	for i, impl in ipairs(impls2) do
		local line = "   "..impl.key.." ("..type(impl.class[impl.key])..")"
		if impl.class ~= class then
			line = line .. " - from "..impl.class.__name
		end
		add(line)
	end
end

