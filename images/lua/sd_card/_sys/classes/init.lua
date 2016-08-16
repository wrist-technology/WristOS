--Initialize public classes

local function load_classes(classes)
	for i,classname in ipairs(classes) do
		local filename = dynawa.dir.sys.."classes/"..classname..".lua"
		dynawa.busy(i/2/#classes)
		log("Compiling class "..classname.."...")
		local class = dofile(filename)
		assert(class, "Class file "..filename.." returned nothing")
		assert(class:_is_class(), "Class file "..filename.." did not return a class but "..tostring(class))
		assert(class:_name() == classname)
		if dynawa.debug and dynawa.debug.pc_classinfo then
			dynawa.debug.pc_classinfo(class)
		end
	end
end

local function class_check()
	--Class
	assert(Class:_name() == "Class")
	assert(Class:_is_class())
	assert(Class:_class() == Class)

	--Object
	local Object = Class("Object")
	local object = Object()

	assert(Object:_name() == "Object")
	assert(Object:_is_class())
	assert(Object:_class() == Class)

	assert(not object:_is_class())
	assert(object:_class() == Object)
	assert(not object:_is_class())
	local stat,err = pcall(object._super,object)
	assert (err:match("called on instance"))

	local Bitmap = Class("Bitmap", Object)
	Bitmap.typebitmap = "bitmap"

	function Bitmap:is_bitmap()
		return true
	end

	function Bitmap:_init()
		self.bitmap_init = true
	end

	local bitmap = Bitmap()
	assert(bitmap.bitmap_init)
	assert(bitmap:is_bitmap())
	assert(type(bitmap.handle_event == "function"))
	----------------------------
	assert("x"..Class == "x[class Class]")
	assert("x"..Bitmap == "x[class Bitmap]")
	assert("x"..Object == "x[class Object]")
	assert("x"..bitmap == "x[instance of Bitmap]")
	assert(("x"..Class()):match("class AnonymousClass"))

	local AnotherBitmap = Class("AnotherBitmap", Bitmap)
	local anotherBitmap = AnotherBitmap()

	assert(AnotherBitmap:_name() == "AnotherBitmap")
	assert(AnotherBitmap:_class() == Class)
	assert(anotherBitmap:_class() == AnotherBitmap)
	----------------------------

	local YetAnotherBitmap = Class("YetAnotherBitmap", AnotherBitmap)

	local yetAnotherBitmap = YetAnotherBitmap()
	assert(yetAnotherBitmap:is_bitmap())
	log("Class check OK!")
end

load_classes{
	"Class",
	"EventSource",
	"Bluetooth",
	"Buttons",
	"Window",
	"MenuItem",
	"Menu",
	"Timers",
	"BluetoothSocket",
	"App",
	"BluetoothApp",
	"AppManager",
	"Vibrator",
	"Battery",
	"Accelerometer",
}

if dynawa.debug then
	class_check()
end

