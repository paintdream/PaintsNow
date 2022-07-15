local User = record
	Id : string
	Host : string
	Target : string
end

local TcpWalker = record
	Dispatcher : WorkDispatcher
	LoginPath : string
	Host : string
	Path : string
	Id : string
	Password : string
	Cookie : string
	Loggedin : boolean
	OnlineUsers : { User }

	__index : any
end

TcpWalker.__index = TcpWalker

local Json = require("Engine/Model/Json")
local Core = require("Engine/Core")

local initUsers : { User } = {}

function TcpWalker:Write(connection : Connection, options : { { string }})
	table.insert(options, { "action", "tcpwalker_request" })
	local url = EchoLegend.MakeUri(self.Id, self.Host, self.Path, options, "")
	-- print(url)
	connection:WriteConnectionHttpRequest(url, "GET", { { "cookie", self.Cookie } }, "")
end

local HttpRequestRecord = record
	Uri : string
	Method : string
	Header : { { string } }
	Data : string
end

local HttpResponseRecord = record
	Status : number | string
	Data : any
	Info : string
end

function TcpWalker:Read(connection : Connection) : HttpResponseRecord
	local result = connection:ReadConnectionHttpRequest() as HttpRequestRecord
	local data = result.Data
	if #data < 10 then -- wp auth failed!
		return { Status = "ERROR", Info = "Expired" }
	else
		return Json.Parse(data) as HttpResponseRecord
	end
end

function TcpWalker:Request(options : { { string } }) : HttpResponseRecord
	local instance = coroutine.running()
	local connection = self.Dispatcher:OpenConnection(self.Host, true, function (connection : Connection, status : string | number)
		-- print("Http code: " .. status)
		if status == "Connected" then
			self:Write(connection, options)
		elseif type(status) == "number" and status as number >= 200 and status as number <= 499 then
			coroutine.resume(instance, self:Read(connection))
		end
	end, false)

	connection:ActivateConnection()
	return coroutine.yield() as HttpResponseRecord
end

function TcpWalker:Enumerate()
	-- print("Enumerate Tcpwalker users ...")
	local object = self:Request({ { "Action", "Enumerate" } })
	-- print("Reading result...")
	if object and object.Status ~= "ERROR" and object.Data then
		-- scan for active connections
		local onlineUsers : { User } = {}
		-- print("Got data" .. #object.data)
		for _, v in ipairs(object.Data as { { string : string }}) do
			local user : User = {
				Id = v["0"],
				Host = v["1"] .. ":" .. v["2"],
				Target = v["3"],
			}

			table.insert(onlineUsers, user)
		end

		self.OnlineUsers = onlineUsers
	end
end

function TcpWalker:Connect(id : string) : HttpResponseRecord
	return self:Request({ { "Action", "Connect" }, { "Target", id } })
end

function TcpWalker:Disconnect(id : string) : HttpResponseRecord
 	return self:Request({ { "Action", "Disconnect" }, { "Target", id } })
end

function TcpWalker:LoginCallback(instance : thread, connection : Connection, uri : string, method : string, header : { { string } }, data : string)
	-- print("HEADER SIZE: " .. #header)
	local allCookie = ""
	for _, item in ipairs(header as {{ string }}) do
		local key = item[1]
		local cookie = item[2]
		if key == "Set-Cookie" then
			local c = string.sub(cookie, 1, (string.find(cookie, ";")))
			if not c:find("WP+Cookie+check;", 1, true) then
				allCookie = allCookie .. c
			end
		end
	end

	if #allCookie ~= 0 then
		-- print("Action login ... " .. allCookie)
		self:Write(connection, { { "Action", "Login" } })
		self.Cookie = allCookie
		self.Loggedin = true
	else
		-- print("TcpWalker connection established, but not authorized.")
		coroutine.resume(instance)
	end
end

local ConnectionRecord = record
	Source : string
	Destination : string
end

function TcpWalker:OnConnected(connection : Connection)
	-- print(">>>> PORT " .. srcPort)
	-- print("LOGIN Path: " .. self.LoginPath)
	local data = "log=" .. self.Id .. "&pwd=" .. self.Password
	local headers : { number : any } = {
		{ "Content-Type", "application/x-www-form-urlencoded; charset=UTF-8" },
		{ "Connection", "keep-alive" },
		{ "Host", self.Host },
	}

	-- print("DATA: " .. data)
	connection:WriteConnectionHttpRequest(self.LoginPath, "POST", headers, data)
end

function TcpWalker:Login() : boolean
	local instance = coroutine.running()

	-- only 'Content-Type' and 'Connection' are required for php server
	self.Loggedin = false
	-- print("Connecting " .. self.Host)
	local connection = self.Dispatcher:OpenConnection(self.Host, true, function (connection : Connection, status : string | number)
		if status == "Connected" then
			-- print("Connected")
			self:OnConnected(connection)
		elseif type(status) == "number" and status as number >= 200 and status as number <= 499 then
			-- print("Status " .. tostring(status))
			if not self.Loggedin then
				local res = connection:ReadConnectionHttpRequest() as HttpRequestRecord
				-- print("LOGIN! " .. res.Uri .. " | " .. res.Method .. " | " .. tostring(#res.Header))
				self:LoginCallback(instance, connection, res.Uri, res.Method, res.Header, res.Data)
			else
				coroutine.resume(instance, self:Read(connection))
			end
		end
	end, false)

	connection:ActivateConnection()
	coroutine.yield()

	return self.Loggedin
end

local UriInfo = record
	User : string
	Host : string
	Path : string
	Fragment : string
	Query : {{ string }}
end

function TcpWalker.New(dispatcher : WorkDispatcher, login : string, url : string, id : string, password : string) : TcpWalker
	local part = EchoLegend.ParseUri(url) as UriInfo
	local lg = EchoLegend.ParseUri(login) as UriInfo

	return setmetatable({
		Dispatcher = dispatcher,
		LoginPath = lg.Path,
		Host = part.Host,
		Path = part.Path,
		Id = id,
		Password = password,
		OnlineUsers = {},
		Loggedin = false
	}, TcpWalker as METATABLE) as TcpWalker
end

return TcpWalker
