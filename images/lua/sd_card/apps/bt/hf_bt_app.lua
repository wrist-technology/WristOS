app.name = "*OLD HandsFree BT"
app.id = "dynawa.hf"

local SDP = Class.BluetoothSocket.SDP

local service = {
    {
        SDP.UINT16(0x0000), -- Service record handle attribute
        SDP.UINT32(0x00000000)
    },
    {
        SDP.UINT16(0x0001), -- Service class ID list attribute
        {
---[[ FF HTC Desire
            SDP.UUID16(0x1108), -- Head-Set
            SDP.UUID16(0x110b), -- Audio Sink
--]]

            SDP.UUID16(0x111e), -- Hands-Free
            SDP.UUID16(0x1203) -- Generic Audio
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
        SDP.UINT16(0x0009), -- Profile descriptor list attribute
        {
            {
                SDP.UUID16(0x111e), -- Hands-Free
                SDP.UINT16(0x0105) -- Version
            }
        }
    }
--[[
    {
        SDP.UINT16(0x0005), -- Browse group list
        {
            SDP.UUID16(0x1002) -- PublicBrowseGroup
        }
    }
--]]
}

app.parser_state_machine = {
    [1] = {
        {"+BRSF:", nil, nil, nil},
        {"OK", 2, "AT+CIND=?\r", nil},
    },
    [2] = {
        {"%+CIND:", nil, nil,  
            -- +CIND: ("service",(0-1)),("call",(0-1)),("callsetup",(0-3)),("callheld",(0->)),("signal",(0-5)),("roam",(0-1)),("battchg",(0-5))
            function(app, socket, data)
                for ind, val_min, val_max in string.gfind(data, '%("(%a+)",%(([^%-,]+)[%-,]([^%)]+)%)%)') do
                    log("ind <" .. ind .. "> min " .. val_min .. " max " .. val_max)
                    table.insert(socket.indicators, {ind, val_min, val_max})
                end 
            end
        },
        {"OK", 3, "AT+CIND?\r", nil},
    },
    [3] = {
        {"%+CIND:", nil, nil,
            -- +CIND: 1,0,0,0,4,0,4
            function(app, socket, data)
                local ind_index = 1
                for val in string.gfind(data, "%d+") do
                    local ind_data = socket.indicators[ind_index]
                    if ind_data then
                        log("ind <" .. ind_data[1] .. "> val " .. val)
                    else
                        log("ind " .. ind_index .. " val " .. val)
                    end 
                    ind_index = ind_index + 1
                end
            end
        },
        {"OK", 4, "AT+CMER=3,0,0,1\r", nil},
    },
    [4] = {
        {"OK", 5, "AT+CLIP=1\r", nil},
    },
    [5] = {
        {"OK", 6, nil, nil},
    },
    [6] = {
        {"+CIEV:", nil, nil, 
            -- +CIEV: 3,1
            function(app, socket, data)
                local ind_index, val = string.match(data, '(%d+),(%d+)')
                local ind_data = socket.indicators[0 + ind_index]
                if ind_data then
                    local ind_handler = app.ind_handlers[ind_data[1]]
                    if ind_handler then
                        ind_handler(app, socket, ind_data[1], val)
                    else
                        log("no handler for indicator " .. ind_data[1])   
                    end
                else
                    log("invalid indicator index " .. ind_index)   
                end
            end
        },
        {"RING", nil, nil, nil},
        {"+CLIP:", nil, nil,
            -- +CLIP: "5555555555",145
            function(app, socket, data)
                local phone_number, format = string.match(data, '"([^"]*)",(%d+)')
                log("phone number " .. phone_number .. " " .. format)
            end
        },
    },
}

app.default_ind_handler = function(app, socket, ind, val)
        log("ind <" .. ind .. "> val " .. val)
end

app.ind_handlers = {
    service = app.default_ind_handler,
    call = app.default_ind_handler,
    callsetup = app.default_ind_handler,
    callheld = app.default_ind_handler,
    signal = app.default_ind_handler,
    roam = app.default_ind_handler,
    battchg = app.default_ind_handler,
}

function app:log(socket, msg)
    log(msg)
end

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
	log(socket.." connection accepted " .. Class.Bluetooth:mac_string(connection_socket.remote_bdaddr))
	self.num_activities = self.num_activities + 1

    connection_socket.parser_state = 1
    connection_socket.indicators = {}
end

function app:handle_event_socket_data(socket, data_in)
    local data_out = {}

    if not data_in then
        self:log(socket, "got empty data")
        --table.insert(data_out, "OK")
    else
        self:log(socket, string.format("got: %q",data_in))
        for line in string.gfind(data_in, "[^\r\n]+") do
            local state_transitions = self.parser_state_machine[socket.parser_state]
            if state_transitions then
                for i, transition in ipairs(state_transitions) do
                    if string.match(line, transition[1]) then
                        -- next state
                        if transition[2] then
                            self:log(socket, "state " .. socket.parser_state .. " -> " .. transition[2])
                            socket.parser_state = transition[2]
                        end
                        -- response string
                        if transition[3] then
                            table.insert(data_out, transition[3])
                        end
                        -- state handler
                        if transition[4] then
                            transition[4](self, socket, line)
                        end
                        break
                    end
                end
            end
        end
    end
    if #data_out > 0 then
        local response = table.concat(data_out)
        self:log(socket, "sending " .. response)
        socket:send(response)
    end
end

function app:handle_event_socket_connected(socket)
    log(socket.." connected")
    socket:send("AT+BRSF=4\r")
end

function app:handle_event_socket_disconnected(socket,prev_state)
	self.num_activities = self.num_activities - 1
end

function app:handle_event_socket_error(socket,error)
end

function app:call_answer(socket)
    socket:send("ATA\r")
end

function app:call_reject(socket)
    socket:send("AT+CHUP\r")
end

function app:status_text()
	return self.num_activities
end
