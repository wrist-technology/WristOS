app.name = "Echo BT"
app.id = "dynawa.echo"

local SDP = Class.BluetoothSocket.SDP

local service = {
    {
        SDP.UINT16(0x0000), -- Service record handle attribute
        SDP.UINT32(0x00000000)
    },
    {
        SDP.UINT16(0x0001), -- Service class ID list attribute
        {
            SDP.UUID16(0x111e) -- Handsfree
        }
    },
    {
        SDP.UINT16(0x0004), -- Protocol descriptor list attribute
        {
            {
                SDP.UUID16(0x0100), -- L2CAP
            },
            {
                SDP.UUID16(0x0003), -- RFCOMM
                SDP.RFCOMM_CHANNEL() -- channel
            }
        }
    },
    {
        SDP.UINT16(0x0005), -- Browse group list
        {
            SDP.UUID16(0x1002) -- PublicBrowseGroup
        }
    }
}

function app:start()
	self.activities = {}
	self.socket = nil
	self.num_activities = 0

	dynawa.bluetooth_manager.events:register_for_events(self)

	if dynawa.bluetooth_manager.hw_status == "on" then
		self:server_start()
	end
end

function app:server_start()
	local socket = assert(self:new_socket("rfcomm"))
	self.socket = socket
	socket:listen(nil)
	--socket:listen(1)
	socket:advertise_service(service)
end

function app:server_stop()
	self.socket:close()
	self.socket = nil
end

function app:handle_bt_event_turned_on()
	self:server_start()
end

function app:handle_bt_event_turning_off()
	self:server_stop()
end

function app:handle_event_socket_connection_accepted(socket, connection_socket)
	log(socket.." connection accepted " .. connection_socket)
	self.num_activities = self.num_activities + 1
end

function app:handle_event_socket_data(socket, data_in)
	assert(data_in)
	log("echo: " .. data_in)
	socket:send(data_in)
end

function app:handle_event_socket_disconnected(socket,prev_state)
	self.num_activities = self.num_activities - 1
end

function app:handle_event_socket_error(socket,error)
end

function app:status_text()
	return self.num_activities
end
