--This is the primary entry point into Lua!
--DO NOT MODIFY THIS FILE.
--Use "override.lua" to tune-up the boot process.

assert(dynawa, "Dynawa library not detected")
math.randomseed(os.time())
dynawa.bitmap.show(dynawa.bitmap.new(160,128,0,255,0))

--Note the following function can be called more than once because of "soft reboot".
--The flag "dynawa.already_booted" can be used to determine if this is the case.
function _G.boot_init()
	dynawa.debug = nil
	
	--release any timers which were active sice the last run
	if dynawa.devices and dynawa.devices.timers and dynawa.devices.timers.timer_vectors then
		for k,v in pairs(dynawa.devices.timers.timer_vectors) do
			dynawa.timer.cancel(k)
		end
	end
	
	rawset(_G,"my",nil)
	dynawa.dir = {root="/"}
	dynawa.dir.sys=dynawa.dir.root .. "_sys/"	
	dynawa.dir.apps=dynawa.dir.root .. "apps/"
	
	_G.log = function() end
	
	local fd = io.open("no_override.lua")
	
	if fd then
		fd:close()
		os.remove("no_override.lua")
	else
		--Try loading the (potential) boot override script stored in the default path
		local override,err = loadfile("/override.lua")
	
		if (not override) and (not (err or ""):match("No such file or directory")) then
			--Override script exists but cannot be loaded
			error(err)
		end
	
		if override then
			--Execute the override script and clear its chunk to save memory
			override()
			override = nil
		end
	end

	_G.handle_event = false
	--Disallow unintended creating of global variables
	local mt=getmetatable(_G)
	if not mt then
		mt={}
		setmetatable(_G,mt)
	end
	mt.__newindex=function(table,key,value)
		error("Attempt to create global value '"..tostring(key).."' (forgot to use 'local'?)",2)
	end

	if dynawa.debug then		--We are in debug mode, use the main loop wrapper
		_G.handle_event = dynawa.debug.main_handler
		dynawa.version=nil 					--Forces the WristOS to be reloaded later in main_handler (by xpcall)
	else									--Not in debug mode
		dofile(dynawa.dir.sys.."wristos.lua") 
		_G.handle_event = _G.private_main_handler
	end
	
	dynawa.already_booted = true
end

--The following dummy definition should be overridden by either override.lua or wristos.lua, if all compiles well
function _G.private_main_handler(message)
	print("DUMMY_PRIVATE_MAIN_HANDLER")
	return nil
end

--Here we go...
boot_init()
--Generate immediate timer event to run the main loop for the first time and actually initialize WristOS in protected mode
dynawa.timer.start(0)

