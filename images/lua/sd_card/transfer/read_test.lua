port = "/dev/ttyUSB2"
fd = assert(io.open(port))
local i=0
while(1) do
	i=i+1
	local str=assert(fd:read("*l"))
	print(i..":<"..#str.."> "..str)
end
