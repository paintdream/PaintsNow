--[[
	Terminal.tl: Simple Network Terminal Wrapper for EchoLegend APIs.
	Author: PaintDream (paintdream@paintdream.com)
]]

local Core = require("Engine/Core")
require("Runtime/Interface")

local ConnectionRecord = record
	Source : string
	Destination : string
	Storage : any
end

local HttpRequestRecord = record
	Uri : string
	Method : string
	Header : { string : string }
	Data : string
end

local Terminal = record
	Peers : { Connection : ConnectionRecord }
	Dispatcher : WorkDispatcher
	Address : string
	Host : Listener
	__index : any
end

Terminal.__index = Terminal

function Terminal.StateHandler(connection : Connection, isConnect : boolean, src : string, dst : string) : boolean
	return true
end

function Terminal.DataHandler(connection : Connection, data : string, method : string, header : { string : string }, extraData : string) : (number, string, { string : string }, string)
	return 200, "", {}, ""
end

function Terminal:Write(connection : Connection, data : string, mode : number)
	connection:WriteConnection(data, mode)
end

function Terminal:Read(connection : Connection) : string
	return connection:ReadConnection()
end

-- Broadcast data to all connected terminals.
function Terminal:Broadcast(data : string, mode : number)
	for k in pairs(self.Peers) do
		self:Write(k, data, mode)
	end
end

-- Add a connection terminal to current terminal
function Terminal:AddConnection(connection : Connection, storage : any)
	local rec = connection:GetConnectionAddresses() as ConnectionRecord
	rec.Storage = storage
	self.Peers[connection] = rec
end

-- Remove a connection terminal from current terminal
function Terminal:RemoveConnection(connection : Connection)
	if connection then
		self.Peers[connection] = nil as ConnectionRecord
	end
end

function Terminal:EventHandler(connection : Connection, status : string, packetData : string) : boolean
	if status == "Connected" then
		local info = connection:GetConnectionAddresses() as ConnectionRecord
		if self.StateHandler(connection, true, info.Source, info.Destination) then
			self:AddConnection(connection, nil)
			return true
		else
			return false
		end
	elseif status == "Read" then
		assert(packetData)
		self.DataHandler(connection, packetData, nil as string, nil as { string : string }, nil as string)
		return true
	elseif status == "Close" or status == "Error" then
		local info = connection:GetConnectionAddresses() as ConnectionRecord
		self.StateHandler(connection, false, info.Source, info.Destination)
		self:RemoveConnection(connection)
		return false
	end
end

function Terminal:Connect(ip : string) : Connection
	local instance = coroutine.running()
	local connected = false
	local success = false
	local connection : Connection = self.Dispatcher:OpenConnection(ip, false, function (connection : Connection, status : string, packetData : string)
		success = self:EventHandler(connection, status, packetData)
		if status == "Connected" then
			connected = true
			if coroutine.status(instance) == "suspended" then
				coroutine.resume(instance, success)
			end
		end
	end, true)

	-- establishing
	connection:ActivateConnection()

	if (connected and success) or (coroutine.yield() as boolean) then
		print("Connect success!")
		self:AddConnection(connection, nil)
		return connection
	else
		print("Connect failed!")
		connection:DeactivateConnection()
		return nil as Connection
	end
end

function Terminal:Disconnect(connection : Connection)
	self:RemoveConnection(connection)
	connection:DeactivateConnection()
end

-- Handle and dispatch http events, it's an internal function.
function Terminal:EventHandlerHttp(connection : Connection, status : string) : boolean
	if status == "Connected" then
		local info = connection:GetConnectionAddresses() as ConnectionRecord
		local storage = self.StateHandler(connection, true, info.Source, info.Destination)
		if storage then
			self:AddConnection(connection, storage)
		else
			self:Disconnect(connection)
		end
	end

	if status == "Read" or status == "Connected" then
		local request = connection:ReadConnectionHttpRequest() as HttpRequestRecord
		local code, reason, header, data = self.DataHandler(connection, request.Uri, request.Method, request.Header, request.Data)
		connection:WriteConnectionHttpResponse(code, reason, header as {any}, data)
		return true
	elseif status == "Close" or status == "Error" then
		local info = connection:GetConnectionAddresses() as ConnectionRecord
		self.StateHandler(connection, false, info.Source, info.Destination)
		self:RemoveConnection(connection)
		return false
	else
		return false
	end
end

function Terminal:HttpRequest(host : string, path : string, method : string,
	options : { string : string }, header : { string : string }, callback : function (string) : (any) ) : Connection
	local connection : Connection = self.Dispatcher:OpenConnection(host, true, function (connection : Connection, status : any)
		local statusCode = tonumber(status) or 0
		if status == "Connected" then
			local opts : { any } = {}
			for k, v in pairs(options) do
				table.insert(opts, { k, v })
			end

			local url = EchoLegend.MakeUri("", "", path, opts, "")
			
			local list : { number : any } = {}
			for k, v in pairs(header) do
				table.insert(list, { k, v })
			end

			connection:WriteConnectionHttpRequest(url, method, list, "")
		elseif type(status) == "number" and statusCode >= 200 and statusCode <= 499 then
			local request = connection:ReadConnectionHttpRequest() as HttpRequestRecord
			callback(request.Data)
		end
	end, true)

	if connection then
		connection:ActivateConnection()
	end

	return connection
end

function Terminal.New(dispatcher : WorkDispatcher, ip : string, 
			dataHandler : any,
			stateHandler : function (Connection, boolean, string, string) : (any),
			http : boolean) : Terminal
	local initEvent = Core.Event()
	local eventHandler = http and Terminal.EventHandlerHttp or Terminal.EventHandler
	local terminal = setmetatable({ IP = ip, Peers = {}, DataHandler = dataHandler, StateHandler = stateHandler, Dispatcher = dispatcher }, Terminal as METATABLE) as Terminal

	if ip then
		local listener : Listener = dispatcher:OpenListener(ip, http, function (listener : Listener, event : string)
			if event == "Connected" then
				local address = listener:GetListenerAddress() as string
				print("Listener created on IP: " .. address)
				Core.Signal(initEvent, address)
			end
		end, function (listener : Listener, connection : Connection)
			-- do not add connection here
			-- wait for actual event
			-- EchoLegend.WriteConnectionRecord(connection, "Hello")
			-- print("NEW CONNECTION")
			eventHandler(terminal, connection, "Connected", nil)
		end, function (connection : Connection, status : string, packetData : string)
			--print("SERVER GOT" .. status)
			eventHandler(terminal, connection, status, packetData)
		end, true)

		if listener then
			listener:ActivateListener()
			print("Listener activated")
			terminal.Address = Core.Wait(initEvent) as string
			terminal.Host = listener
			return terminal
		else
			print("Create listener failed!")
			return nil as Terminal
		end
	else
		return terminal
	end
end

return Terminal