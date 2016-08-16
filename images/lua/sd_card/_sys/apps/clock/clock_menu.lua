--Clock settings
require ("dynawa")

local function show_menu(message)
	local menu = {
		banner = "Clock settings",
		items = {
			{text="Blah",value="no"},
			{text="Blah2",value="no"},
			{text="Blah3",value="no"},
			{text="Very long string 1234567890 really very long, WOW it's totally huge line!",value="no"},
			{text="Very long dark string 1234567890 really very long, WOW it's totally huge line!"},
			{text="BlahCCC",value="no"},
			{text="Blah2",value="no"},
			{text="Blah3"},
			{text="Blah4",value="no"},
			{text="Blah5",value="no"},
		}
	}
	for i = 1, 20 do
		table.insert(menu.items,{text="Dummy item #"..i,value="no"})
	end
	dynawa.message.send{type="new_widget", menu = menu}
end

dynawa.message.receive{message = "show_menu", callback = show_menu}

