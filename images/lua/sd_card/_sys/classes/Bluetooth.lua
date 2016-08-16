local class = Class("Bluetooth", Class.EventSource)

-- constants

class.cod = {
    SPP = string.byte(0x08,0x04,0x24), -- {0x08,0x04,0x24};
    WEARABLE = string.byte(0x00,0x07,0x04), -- {0x00,0x07,0x04};
    HANDSFREE = string.byte(0x20,0x04,0x08)
}

function class:_init()
	Class.EventSource._init(self,"bluetooth")
	local cmd = {
		open = 1,
		close = 2,
		set_link_key = 3,
		inquiry = 4,
		remote_name_req = 5,
		link_key_req_reply = 8,
		link_key_req_neg_reply = 9,
		socket_new = 100,
		socket_close = 101,
		socket_bind = 102,
		find_service = 200,
		listen = 300,
		connect = 301,
		send = 400,
		advertise_service = 500,
		stop_advertising = 501,
	}
	self.cmd = {}
	for key, val in pairs(cmd) do
		self.cmd[key] = function(self, ...)
			return dynawa.bt.cmd(val, ...)
		end
	end
end

local events = {
    [1] = "started",
    [5] = "stopped",
    [10] = "link_key_not",
    [11] = "link_key_req",
    [15] = "connected", --sock
    [16] = "disconnected", --sock
    [17] = "accepted", --sock
    [20] = "data", --sock
    [30] = "find_service_result", --sock
    [40] = "inquiry_complete",
    [41] = "inquiry_result",
    [45] = "remote_name",
    [100] = "error", --sock
}

local socket_events = {connected = true, disconnected = true, accepted = true, data = true, find_service_result = true, error = true}

function class:handle_hw_event(event)
	event.subtype = assert(events[event.subtype],"Unknown BT event: "..event.subtype)
	if socket_events[event.subtype] then --Handled by BluetoothSocket instance
		if not event.socket then
			--#todo what's this???
			log("*** Received BT HW event of subtype "..tostring(event.subtype).." but socket is nil")
		else
			--log("BT sending event '"..event.subtype.."' to "..(event.socket or "nil"))
			event.socket["handle_bt_event_"..event.subtype](event.socket,event)
		end
	else
		self:generate_event(event) --Handled by BT manager
	end
end

function class:mac_string(bdaddr)
    assert(#bdaddr == 6, "MAC should be 6 bytes")
    return string.format("%02x:%02x:%02x:%02x:%02x:%02x", string.byte(bdaddr:reverse(), 1, -1))
end

return class

