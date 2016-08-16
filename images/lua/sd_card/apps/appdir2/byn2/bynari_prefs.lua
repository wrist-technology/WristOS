--Bynari Clock settings
require ("dynawa")

local after_select = {popup = "Color scheme changed", close_menu = true}

local menu_result = function(message)
	assert(message.value)
	if my.globals.prefs.style ~= message.value then
		my.globals.prefs.style = message.value
		dynawa.file.save_data(my.globals.prefs)
	end
end

local your_menu = function(message)
	local menu = {
		banner = "Bynari color schemes",
		active_value = assert(my.globals.prefs.style),
		items = {
			{
				text = "Rainbow", value = "default", after_select = after_select
			},
			{
				text = "Planet Earth", value = "blue/green", after_select = after_select
			},
			{
				text = "Pure snow", value = "white", after_select = after_select
			},
			{
				text = "Inferno", value = "red", after_select = after_select
			},

			{
				text = "Rainbow", value = "default", after_select = after_select
			},
			{
				text = "Planet Earth", value = "blue/green", after_select = after_select
			},
			{
				text = "Pure snow", value = "white", after_select = after_select
			},
			{
				text = "Inferno", value = "red", after_select = after_select
			},
			{
				text = "Rainbow", value = "default", after_select = after_select
			},
			{
				text = "Planet Earth", value = "blue/green", after_select = after_select
			},
			{
				text = "Pure snow", value = "white", after_select = after_select
			},
			{
				text = "Inferno", value = "red", after_select = after_select
			},
			{
				text = "Rainbow"
			},
			{
				text = "Planet Earth"
			},
			{
				text = "Pure snow", value = "white", after_select = after_select
			},
			{
				bitmap = dynawa.bitmap.new(99,25,255,0,0), value = "red", after_select = after_select
			},
			{
				text = "Rainbow", value = "default", after_select = after_select
			},
			{
				text = "Planet Earth", value = "blue/green", after_select = after_select
			},
			{
				text = "Pure snow", value = "white", after_select = after_select
			},
			{
				text = "Inferno", value = "red", after_select = after_select
			},

		},
	}
	return menu
end

dynawa.message.receive{message = "your_menu", callback = your_menu}
dynawa.message.receive{message = "menu_result", callback = menu_result}

