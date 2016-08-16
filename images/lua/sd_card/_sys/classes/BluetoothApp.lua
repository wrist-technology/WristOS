local class = Class("BluetoothApp", Class.App)

function class:_init(...)
	Class.App._init(self,...)
end

--In the case of BT Apps, this is also called whenever the App's name in BT Manager's App list is clicked
--Sensible default is to return the BT App's Activities list
function class:handle_event_do_menu()
	local menudesc = {banner = self.name.." activities:"}
	local items = assert(self:activity_items())
	menudesc.items = items
	self:new_menuwindow(menudesc):push()
end

--This should return a SHORT textual representation of BT App's current status.
--Each BT App must override this to at least return the empty string (i.e. status is irrelevant).
function class:status_text()
	return ("NO_STATUS")
end

--Returns an arrray of Activities for the given App. Array can be empty.
function class:activity_items()
	return {}
end

local protocol_codes = {
	hci = 1,
	l2cap = 2,
	sdp = 3,
	rfcomm = 4,
}

function class:new_socket(protocol)
	local proto = protocol_codes[protocol]
	if not proto then
		error("Unknown BT socket protocol: "..protocol)
	end
	local sock = Class.BluetoothSocket(proto)
	sock.app = self
	return sock
end

function class:handle_event_socket_connected(socket)
	log(socket.." connected, DOING NOTHING")
end

function class:handle_event_socket_connection_accepted(socket, connection_socket)
	log(socket.." connection accepted " .. connection_socket..", DOING NOTHING")
end

function class:handle_event_socket_data(socket,data)
	log(string.format("%s got %s bytes of data: '%q', DISCARDING", tostring(socket), #data, data))
end

function class:handle_event_socket_disconnected(socket)
	log(socket.." disconnected, DOING NOTHING")
end

function class:handle_event_socket_error(socket,error)
	error("BT error event '"..tostring(error).."' in "..socket..", IGNORING")
end

function class:handle_event_socket_find_service_result(socket,channel)
	log ("Find_service_result channel = "..tostring(channel)..", IGNORING")
end

--For the following event to be received, BT App must subscribe to dynawa.bluetooth_manager.events
function class:handle_event_new_paired_device(event)
end

--For the following event to be received, BT App must subscribe to dynawa.bluetooth_manager.events
function class:handle_event_removed_paired_device(event)
end

function class:handle_bt_event_turned_on()
	--BT hardware was just turned on, i.e. it IS already up and running at this moment.
end

function class:handle_bt_event_turning_off()
	--BT hardware *WILL BE* turned off right now. Act accordingly. The BT is still running but there is no time to send any "last piece of data", just shut everything down cleanly.
end

return class

