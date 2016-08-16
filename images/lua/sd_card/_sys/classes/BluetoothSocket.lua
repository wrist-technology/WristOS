local class = Class("BluetoothSocket")
class.is_bluetooth_socket = true

--[[
#define SDP_DE_TYPE_NIL     (0<<3) /* Nil, the null type */
#define SDP_DE_TYPE_UINT    (1<<3) /* Unsigned Integer */
#define SDP_DE_TYPE_STCI    (2<<3) /* Signed, twos-complement integer */
#define SDP_DE_TYPE_UUID    (3<<3) /* UUID, a universally unique identifier */
#define SDP_DE_TYPE_STR     (4<<3) /* Text string */
#define SDP_DE_TYPE_BOOL    (5<<3) /* Boolean */
#define SDP_DE_TYPE_DES     (6<<3) /* Data Element Sequence */
#define SDP_DE_TYPE_DEA     (7<<3) /* Data Element Alternative */
#define SDP_DE_TYPE_URL     (8<<3) /* URL, a uniform resource locator */

#define SDP_DE_SIZE_8   0x0 /* 8 bit integer value */
#define SDP_DE_SIZE_16  0x1 /* 16 bit integer value */
#define SDP_DE_SIZE_32  0x2 /* 32 bit integer value */
#define SDP_DE_SIZE_64  0x3 /* 64 bit integer value */
#define SDP_DE_SIZE_128 0x4 /* 128 bit integer value */
#define SDP_DE_SIZE_N1  0x5 /* Data size is in next 1 byte */
#define SDP_DE_SIZE_N2  0x6 /* Data size is in next 2 bytes */
#define SDP_DE_SIZE_N4  0x7 /* Data size is in next 4 bytes */

#define SDP_DES_SIZE8   (SDP_DE_TYPE_DES  | SDP_DE_SIZE_N1)
#define SDP_DES_SIZE16  (SDP_DE_TYPE_DES  | SDP_DE_SIZE_N2)
#define SDP_UINT8       (SDP_DE_TYPE_UINT | SDP_DE_SIZE_8)
#define SDP_UINT16      (SDP_DE_TYPE_UINT | SDP_DE_SIZE_16)
#define SDP_UINT32      (SDP_DE_TYPE_UINT | SDP_DE_SIZE_32)
#define SDP_UUID16      (SDP_DE_TYPE_UUID | SDP_DE_SIZE_16)
#define SDP_UUID128     (SDP_DE_TYPE_UUID | SDP_DE_SIZE_128)
--]]

class.SDP = {
	dividers = {
		256, 256 * 256, 256 * 256 * 256
	},

	num2bytes = function(self, num, num_bytes) 
		local bytes = {}
		local i = num_bytes - 1
		while i > 0 do
			local divider = self.dividers[i]
			local b = math.floor(num / divider)
			--log("num2bytes " .. i .. " " .. num .. " " .. b)
			table.insert(bytes, string.char(b))
			num = num % divider
			i = i - 1
		end
		table.insert(bytes, string.char(num))
		return table.concat(bytes)
	end,

	constructor_value = function(self, value)
		local t = {
			value = value
		}
		setmetatable(t, self)
		return t
	end,

	constructor_void = function(self)
		local t = {
		}
		setmetatable(t, self)
		return t
	end,

	tostring_value = function(self)
		return self.type .. "(" .. self.value .. ")"
	end,

	set_metamethods = function(self, type, t)
		--log(tostring(self) .. " " .. type .. " " .. tostring(t))
		self[type] = t
		t.__index = t
		t.type = type
		setmetatable(t, t)
	end,

	des = function(self, t, socket)
		assert(type(t) == "table")
		log("sdp.des")
		local des_r = {}
		for i,v in ipairs(t) do
			if v.type then
				log("sdp." .. tostring(v))
				table.insert(des_r, v.serialize(v, socket))
			else
				table.insert(des_r, self:des(v, socket))
			end
		end
		local des_s = table.concat(des_r)
		local len = string.len(des_s)
		return string.char(48 + 5) .. string.char(len) .. des_s
	end,

	init = function(self)
		--log(self)
		self:set_metamethods("UINT8", {
			__tostring = self.tostring_value,
			__call = self.constructor_value,
			serialize = function(t, socket)
				return string.char(8 + 0) .. self:num2bytes(t.value, 1)
			end
		})
		self:set_metamethods("UINT16", {
			__tostring = self.tostring_value,
			__call = self.constructor_value,
			serialize = function(t, socket)
				return string.char(8 + 1) .. self:num2bytes(t.value, 2)
			end
		})
		self:set_metamethods("UINT32", {
			__tostring = self.tostring_value,
			__call = self.constructor_value,
			serialize = function(t, socket)
				return string.char(8 + 2) .. self:num2bytes(t.value, 4)
			end
		})

		self:set_metamethods("UUID16", {
			__tostring = self.tostring_value,
			__call = self.constructor_value,
			serialize = function(t, socket)
				return string.char(24 + 1) .. self:num2bytes(t.value, 2)
			end
		})

		self:set_metamethods("BOOL", {
			--__tostring = self.tostring_value,
			__tostring = function(t)
				return t.type
			end,
			__call = self.constructor_value,
			serialize = function(t, socket)
				-- return string.char(40 + (t.value and 1 or 0))
                local val;
                if t.value then
                    val = 1
                else
                    val = 0
                end
				return string.char(40 + val)
			end
		})

		self:set_metamethods("STR8", {
			__tostring = self.tostring_value,
			__call = self.constructor_value,
			serialize = function(t, socket)
				return string.char(32 + 5) .. self:num2bytes(string.len(t.value), 1) .. t.value
			end
		})

		self:set_metamethods("RFCOMM_CHANNEL", {
			__tostring = function(t)
				return t.type
			end,
			__call = self.constructor_void,
			serialize = function(t, socket)
				--log("RFCOMM_CHANNEL " .. socket.channel)
				return string.char(8 + 0) .. string.char(socket.channel)
			end
		})
	end
}

class.SDP:init()

function class:_init(protocol, accept_event)
	self.proto = assert(protocol)
	self.id = dynawa.unique_id()
	if accept_event then
        local c_socket = assert(accept_event.client_socket)
        local remote_bdaddr = assert(accept_event.remote_bdaddr)
		dynawa.devices.bluetooth.cmd:socket_bind(self, c_socket)
		self._c = c_socket
        self.remote_bdaddr = remote_bdaddr
	else
		self._c = dynawa.devices.bluetooth.cmd:socket_new(self)
	end
	self.state = "initialized"
	log("Initialized new socket: "..self)
	return self
end

function class:close()
	local dbgtxt = "Closed and deleted "..self
	dynawa.devices.bluetooth.cmd:socket_close(self._c)
	self:_delete()
	log(dbgtxt)
end

function class:connect(bdaddr, channel)
	log(self.." connecting at channel "..channel)
	dynawa.devices.bluetooth.cmd:connect(self._c, bdaddr, channel)
    self.remote_bdaddr = bdaddr
end

function class:listen(channel)
	self.state = "listening"
	self.channel = dynawa.devices.bluetooth.cmd:listen(self._c, channel or 0)
end


function class:advertise_service(service)
	assert(self.state == "listening")

	log("advertise_service " .. self.channel)
	local record = {}
	for i,v in ipairs(service) do
		table.insert(record, self.SDP:des(v, self))
	end
	local record_bin = table.concat(record)

	dynawa.devices.bluetooth.cmd:advertise_service(self._c, record_bin)
end

function class:send(data)
	--log(self.." sending "..#data.." bytes of raw data")
	dynawa.devices.bluetooth.cmd:send(self._c, data)
end

function class:handle_bt_event_connected(event)
	self.state = "connected"
	self.app:handle_event_socket_connected(self)
end

function class:handle_bt_event_disconnected(event)
	local prvstate = self.state
	self.state = "disconnected"
	self.app:handle_event_socket_disconnected(self,prvstate)
end

function class:handle_bt_event_accepted(event)
	local sock = Class.BluetoothSocket(self.proto, assert(event))
	sock.app = self.app
	sock.state = "connected"
	self.app:handle_event_socket_connection_accepted(self, sock)
	self.app:handle_event_socket_connected(sock)
end

function class:handle_bt_event_data(event)
	if event.data then
		self.app:handle_event_socket_data(self, assert(event.data))
	else
		log("WARNING: Socket 'incoming data' event's 'data' is nil")
	end
end

function class:handle_bt_event_find_service_result(event)
	local channel = event.channel
	local app = self.app
	app:handle_event_socket_find_service_result(self,channel)
	self:close()
end

function class:handle_bt_event_error(event)
	self.app:handle_event_socket_error(self, event.error)
end

return class

