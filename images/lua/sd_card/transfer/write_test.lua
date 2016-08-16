port = "/dev/ttyUSB0"
fd = assert(io.open(port,"w+"))
local i=0
while(1) do
	local str=io.read()
	str=str.."\n"
	print(#str.." chars")
	fd:write(str)
	fd:flush()
end
