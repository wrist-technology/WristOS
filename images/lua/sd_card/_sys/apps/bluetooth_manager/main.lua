require("dynawa")


local event = {
    BT_STARTED = 1,
    BT_STOPPED = 5,
    BT_LINK_KEY_NOT = 10,
    BT_LINK_KEY_REQ = 11,
    BT_CONNECTED = 15,
    BT_DISCONNECTED = 16,
    BT_ACCEPTED = 17,
    BT_DATA = 20,
    BT_FIND_SERVICE_RESULT = 30,
    BT_ERROR = 100,
}

-- BT API begin

local cmd = {
    OPEN = 1,
    CLOSE = 2,
    SET_LINK_KEY = 3,
    INQUIRY = 4,
    LINK_KEY_REQ_REPLY = 8,
    LINK_KEY_REQ_NEG_REPLY = 9,
    SOCKET_NEW = 100,
    SOCKET_CLOSE = 101,
    SOCKET_BIND = 102,
    FIND_SERVICE = 200,
    LISTEN = 300,
    CONNECT = 301,
    SEND = 400,
}

local function bdaddr2str(bdaddr)
    return string.format("%02x:%02x:%02x:%02x:%02x:%02x", string.byte(bdaddr, 1, -1))
end


-- dynawa.bt.socket begin

local bt_socket = {
    -- constants
    -- socket protocol
    PROTO_HCI = 1,
    PROTO_L2CAP = 2,
    PROTO_SDP = 3,  -- hack
    PROTO_RFCOMM = 4,

    -- socket state
    STATE_INITIALIZED = 1,
    STATE_LISTENING = 2,
    STATE_CONNECTED = 3,
    STATE_DISCONNECTED = 4,

    -- variables
    socket_count = 0,


    -- functions

    -- public
    listen = function(socket, channel)
        dynawa.bt.cmd(cmd.LISTEN, socket._c, channel)
    end,
    advertise_service = function(socket, service)
    end,
    connect = function(socket, bdaddr, channel)
        dynawa.bt.cmd(cmd.CONNECT, socket._c, bdaddr, channel)
    end,
    close = function(socket)
        dynawa.bt.cmd(cmd.SOCKET_CLOSE, socket._c)
    end,
    send = function(socket, data)
        dynawa.bt.cmd(cmd.SEND, socket._c, data)
    end,
    event = function(socket, event_id, event_data)
        socket.event_handler(socket.event_handler_data, socket, event_id, event_data) 
    end,
}


-- outside of the bt_socket table because of use of bt_socket.slot  ... 

bt_socket._new = function(socket_proto, socket_event_handler, socket_event_handler_data) 
    assert(socket_proto == bt_socket.PROTO_RFCOMM or socket_proto == bt_socket.PROTO_SDP, "Socket proto" .. socket_proto .. " not implemented")
    assert(socket_event_handler, "socket: no event handler")
    bt_socket.socket_count = bt_socket.socket_count + 1
    local socket = {
        id = bt_socket.socket_count,

        proto = socket_proto,
        event_handler = socket_event_handler,
        event_handler_data = socket_event_handler_data,

        state = bt_socket.STATE_INITIALIZED,
    }
    return socket
end

bt_socket.new = function(socket_proto, socket_event_handler, socket_event_handler_data) 
    assert(socket_proto == bt_socket.PROTO_RFCOMM or socket_proto == bt_socket.PROTO_SDP, "Socket proto" .. socket_proto .. " not implemented")
    assert(socket_event_handler, "socket: no event handler")
    bt_socket.socket_count = bt_socket.socket_count + 1
    local socket = bt_socket._new(socket_proto, socket_event_handler, socket_event_handler_data)
    socket._c = dynawa.bt.cmd(cmd.SOCKET_NEW, socket)    -- allocate C socket
    return socket 
end

bt_socket._new_from_c = function(socket, _c_new_socket)
    local socket_new = bt_socket._new(socket.proto, socket.event_handler, socket.event_handler_data)
    dynawa.bt.cmd(cmd.SOCKET_BIND, socket_new, _c_new_socket)    --bind Lua & C socket
    socket_new._c = _c_new_socket
    return socket_new 
end


dynawa.bt.socket = bt_socket

-- dynawa.bt.socket end

local function set_link_key(bdaddr, link_key)
    dynawa.bt.cmd(cmd.SET_LINK_KEY, bdaddr, link_key)
end

local function link_key_req_reply(bdaddr, link_key)
    dynawa.bt.cmd(cmd.LINK_KEY_REQ_REPLY, bdaddr, link_key)
end

local function link_key_req_neg_reply(bdaddr)
    dynawa.bt.cmd(cmd.LINK_KEY_REQ_NEG_REPLY, bdaddr)
end

local function find_service(bdaddr, event_handler, event_handler_data)
    local socket = bt_socket.new(bt_socket.PROTO_SDP, event_handler, event_handler_data)
    dynawa.bt.cmd(cmd.FIND_SERVICE, socket._c, bdaddr)
end

local function inquiry()
    dynawa.bt.cmd(cmd.INQUIRY)
end

-- BT API end

-- SE MBW-150 emulator


local mbw150 = {

    -- private variables

    parser_state_machine = {
        [1] = {
            {"%*SEAM", 2, "AT*SEAUDIO=0,0\r", nil},
            {"OK", 2, "AT*SEAUDIO=0,0\r", nil},
        },
        [2] = {
            {"ERR", 3, "AT+CIND=?\r", nil},
        },
        [3] = {
            {"%+CIND:", 4, "AT+CIND?\r",  nil},
        },
        [4] = {
            {"%+CIND:", 5, "AT+CMER=3,0,0,1\r", nil},
        },
        [5] = {
            {"OK", 6, "AT+CCWA=1\r", nil},
        },
        [6] = {
            {"OK", 7, "AT+CLIP=1\r", nil},
        },
        [7] = {
            {"OK", 8, "AT+GCLIP=1\r", nil},
        },
        [8] = {
            {"OK", 9, "AT+CSCS=\"UTF-8\"\r", nil},
        },
        [9] = {
            {"OK", 10, "AT*SEMMIR=2\r", nil},
        },
        [10] = {
            {"OK", 11, "AT*SEVOL?\r", nil},
        },
        [11] = {
            {"SEVOL", 12, "ATE0\r", nil},
        },
        [12] = {
            {"OK", 13, "AT+CCLK?\r", nil},
        },
        [13] = {
            --{"CCLK", 14, nil, nil},
            {"CCLK", 14, nil, 
                function(connection, data)
                    -- example: +CCLK: "2010/03/11,23:45:14+00"
                    local year, month, day, hour, min, sec = string.match(data, "CCLK: \"(%d+)/(%d+)/(%d+),(%d+):(%d+):(%d+)")
                    local time = os.time({["year"]=year, ["month"]=month, ["day"]=day, ["hour"]=hour, ["min"]=min, ["sec"]=sec})
                    log("time " .. time)
                    dynawa.time.set(time)
                end 
            },
        },
        --{ "X", "AT+CHUP" },
    },

    -- functions

    log = function(connection, message)
        log(connection.bdaddr_str .. ": " .. message)
    end,

    -- callbacks

    reconnect_attempt = function(message)
        local connection = assert(message.connection)
        local handler = connection.handler
        find_service(connection.bdaddr, handler.find_service_events, connection)
    end,

    reconnect = function(connection)
        local handler = connection.handler
        connection.connection_attempt = connection.connection_attempt + 1
        local when = connection.connection_attempt * 60
        handler.log(connection, "attempt #" .. connection.connection_attempt .. " to reconnect in " .. when .. "s")
        dynawa.delayed_callback{time = when * 1000, callback=handler.reconnect_attempt, ["connection"] = connection}
    end,

    socket_events = function(connection, socket, event_id, message)
        local handler = connection.handler
        if event_id == event.BT_CONNECTED then
            handler.log(connection, "connected")
            connection.connection_attempt = 0
            connection.parser_state = 1 
            local data_out = "AT*SEAM=\"MBW-150\",13\r"
            bt_socket.send(socket, data_out)
        elseif event_id == event.BT_DISCONNECTED then
            handler.log(connection, "disconnected")
            bt_socket.close(socket) -- possible reuse?
            handler.reconnect(connection)
        elseif event_id == event.BT_DATA then
            local data_in = message.data
            local data_out
            if not data_in then
                handler.log(connection, "got empty data")
                --data_out = "OK"
            else
                handler.log(connection, "got " .. data_in)
                local state_transitions = handler.parser_state_machine[connection.parser_state]
                if state_transitions then
                    for i, transition in ipairs(state_transitions) do
                        if string.match(data_in, transition[1]) then
                            -- next state
                            if transition[2] then
                                handler.log(connection, "state " .. connection.parser_state .. " -> " .. transition[2])
                                connection.parser_state = transition[2]
                            end
                            -- response string
                            if transition[3] then
                                data_out = transition[3]
                            end
                            -- state handler
                            if transition[4] then
                                transition[4](connection, data_in)
                            end
                            break
                        end
                    end
                end
            end
            if data_out then
                handler.log(connection, "sending " .. data_out)
                bt_socket.send(socket, data_out)
            end
        elseif event_id == event.BT_ERROR then
            handler.log(connection, "socket_events: error " .. message.error)
            bt_socket.close(socket) -- possible reuse?
            handler.reconnect(connection)
        else
            assert(false, "socket_events: unknown event " .. event_id)
        end
    end,

    find_service_events = function(connection, socket, event_id, message)
        local handler = connection.handler
        -- TODO: better operation result reporting:
        --  like: == event.BT_OK
        if event_id == event.BT_FIND_SERVICE_RESULT then
            local channel = message.channel
            handler.log(connection, "channel " .. channel)
            if channel > 0 then
                connection.channel = channel
                local socket = bt_socket.new(bt_socket.PROTO_RFCOMM, handler.socket_events, connection)
                connection.socket = socket
                bt_socket.connect(socket, connection.bdaddr, channel)
            else
                handler.log(connection, "find_service_events: no remote listening RFCOMM")
                handler.reconnect(connection)
            end
        elseif event_id == event.BT_DISCONNECTED then
            --TODO: this event shouldn't get here!
            handler.log(connection, "find_service_events: disconnected")
            handler.reconnect(connection)
        elseif event_id == event.BT_ERROR then
            handler.log(connection, "find_service_events: error " .. message.error)
            handler.reconnect(connection)
        else
            assert(false, "find_service_events: unknown event " .. event_id)
        end
    end,

    start = function(connection)
        local handler = connection.handler
        connection.connection_attempt = 0
        connection.bdaddr_str = bdaddr2str(connection.bdaddr)
        find_service(connection.bdaddr, handler.find_service_events, connection)
    end,
}

-- echo (service) plugin

local echo = {

    -- private variables


    -- functions

    log = function(connection, message)
        log("echo: " .. message)
    end,

    -- callbacks

    socket_events = function(connection, socket, event_id, message)
        local handler = connection.handler
        if event_id == event.BT_ACCEPTED then
            handler.log(connection, "accepted")
        elseif event_id == event.BT_DISCONNECTED then
            handler.log(connection, "disconnected")
            bt_socket.close(socket) -- possible reuse?
        elseif event_id == event.BT_DATA then
            local data_in = message.data
            local data_out
            if not data_in then
                handler.log(connection, "got empty data")
                data_out = "OK"
            else
                handler.log(connection, "got " .. data_in)
                data_out = data_in
            end
            if data_out then
                handler.log(connection, "sending " .. data_out)
                bt_socket.send(socket, data_out)
            end
        elseif event_id == event.BT_ERROR then
            handler.log(connection, "socket_events: error " .. message.error)
            --bt_socket.close(socket) -- possible reuse?
        else
            assert(false, "socket_events: unknown event " .. event_id)
        end
    end,

    start = function(connection)
        local handler = connection.handler
        -- listen test
        local socket = bt_socket.new(bt_socket.PROTO_RFCOMM, handler.socket_events, connection)
        bt_socket.listen(socket, 1)
    end,
}

-- BT dispatcher

--local device_bdaddr = string.char(0x37, 0xb0, 0x87, 0xcc, 0x1f, 0x00) -- SGH-I780
local active_connections

local function got_message(message)
    log("Got BT message "..message.subtype)
    --[[
    for k,v in pairs(message) do
        log(tostring(k).."="..tostring(v))
    end
    --]]

    if message.subtype == event.BT_ERROR then
        log("BT_ERROR")
        local socket = message.socket
        assert(socket)
        bt_socket.event(socket, event.BT_ERROR, message)
    elseif message.subtype == event.BT_STARTED then
        log("BT_STARTED")

        active_connections = {}

        if false then
            local connection = {
                handler = echo,
            }

            table.insert(active_connections, connection)
            connection.handler.start(connection)
        end

        if true then
            for bdaddr, link_key in pairs(my.globals.prefs.devices) do
                log("Connecting " .. bdaddr2str(bdaddr))

                set_link_key(bdaddr, link_key)

                local connection = {
                    bdaddr = bdaddr,
                    handler = mbw150,
                }

                table.insert(active_connections, connection)
                connection.handler.start(connection)
            end
        end
    elseif message.subtype == event.BT_STOPPED then
        log("BT_STOPPED")
    elseif message.subtype == event.BT_LINK_KEY_NOT then
        log("BT_LINK_KEY_NOT")
        local bdaddr = message.bdaddr
        local link_key = message.link_key
        log("bdaddr " .. bdaddr2str(bdaddr))
        my.globals.prefs.devices[bdaddr] = link_key
        dynawa.file.save_data(my.globals.prefs)
    elseif message.subtype == event.BT_LINK_KEY_REQ then
        log("EVENT_BT_LINK_KEY_REQ")
        local bdaddr = message.bdaddr
        log("bdaddr " .. bdaddr2str(bdaddr))
        local link_key = my.globals.prefs.devices[bdaddr]
        if link_key then
            link_key_req_reply(bdaddr, link_key)
        else
            link_key_req_neg_reply(bdaddr)
        end
    elseif message.subtype == event.BT_CONNECTED then
        log("BT_CONNECTED")
        local socket = message.socket
        assert(socket)
        socket.state = bt_socket.STATE_CONNECTED
     
        log("socket " .. socket.id)
        bt_socket.event(socket, event.BT_CONNECTED, nil)
    elseif message.subtype == event.BT_DISCONNECTED then
        log("BT_DISCONNECTED")
        local socket = message.socket
        assert(socket)
        socket.state = bt_socket.STATE_DISCONNECTED

        log("socket " .. socket.id)
        bt_socket.event(socket, event.BT_DISCONNECTED, nil)
        --bt_socket.close(socket)
    elseif message.subtype == event.BT_ACCEPTED then
        log("BT_ACCEPTED")
        local socket = message.socket
        assert(socket)
        log("socket " .. socket.id)
        local _c_client_socket = message.client_socket
        assert(_c_client_socket)

        local client_socket = bt_socket._new_from_c(socket, _c_client_socket)
        log("socket " .. client_socket.id)
        client_socket.state = bt_socket.STATE_CONNECTED
        bt_socket.event(socket, event.BT_ACCEPTED, client_socket)
    elseif message.subtype == event.BT_DATA then
        log("BT_DATA")
        local socket = message.socket
        assert(socket)
        --local data = message.data
        log("socket " .. socket.id)
        bt_socket.event(socket, event.BT_DATA, message)
    elseif message.subtype == event.BT_FIND_SERVICE_RESULT then
        log("BT_FIND_SERVICE_RESULT")
        local socket = message.socket
        assert(socket)
        log("socket " .. socket.id)

        bt_socket.event(socket, event.BT_FIND_SERVICE_RESULT, message)
        bt_socket.close(socket)
    end
end

-- wristos menu

local jump = {}

local menu_result = function(message)
    local val = assert(message.value)
    assert(val.jump,"Menu item value has no 'jump'")
    --log("Jump: "..val.jump)
    jump[val.jump](val)
end

local your_menu = function(message)
    local menu = {
        banner = "Bluetooth debug menu",
        items = {
            {
                text = "BT on", value = {jump = "btcmd", command = cmd.OPEN},
            },
            {
                text = "BT off", value = {jump = "btcmd", command = cmd.CLOSE},
            },
            {
                text = "Pairing", value = {jump = "pairing"},
            },
            {
                text = "Something else", value = {jump = "something_else"},
            },
        },
    }
    return menu
end

jump.btcmd = function(args)
    log("Doing bt.cmd "..args.command)
    dynawa.bt.cmd(args.command)
    log("Done")
end

jump.pairing = function(args)
    log("NOT doing pairing...")
end

jump.something_else = function(args)
    log("NOT doing something else")
end

-- initialization

my.app.name = "Bluetooth"
dynawa.message.receive{message = "bluetooth", callback = got_message}
dynawa.message.receive{message = "your_menu", callback = your_menu}
dynawa.message.receive{message = "menu_result", callback = menu_result}
my.globals.prefs = dynawa.file.load_data() or {devices = {}}
