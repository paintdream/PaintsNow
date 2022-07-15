-- DataLive.lua
-- Create data live server

local DataLive = record
	__index : any
end

DataLive.__index = DataLive

local Terminal = require("Engine/Controller/Terminal")

local Request = record
	Code : number
	Method : string
	InputHeader : { string : string }
	InputData : string
	OutputHeader : { string : string }
	OutputData : string
	Reason : string
end

local function DirectContent(mime : string) : function (string, Request, string)
	return function (file : string, request : Request, filePath : string)
		request.OutputHeader["Content-Type"] = mime
		request.OutputData = file
	end
end

local function ExecuteScript(defContent : string, request : Request, filePath : string) : string
	-- trying to read script file
	local path = filePath:match(".*%.")
	if path then
		path = path .. ".lua"
		local content = SnowyStream.FetchFileData(path) as string

		if content and #content ~= 0 then
			local result = ""
			local status, msg = pcall(load(content, "lua"), defContent)
			if not status then
				print("Error: " .. (msg as string))
			else
				result = msg as string
			end

			return result
		end
	end

	return defContent
end

local translators : { string : function (string, Request, string) } = {
	["lua"] = ExecuteScript,
	["json"] = DirectContent("text/json"),
	["html"] = DirectContent("text/html"),
	["js"] = DirectContent("text/javascript"),
	["css"] = DirectContent("text/css"),
	["ico"] = DirectContent("image/x-icon"),
	["png"] = DirectContent("image/png"),
	["jpg"] = DirectContent("image/jpeg"),
	["gif"] = DirectContent("image/gif"),
	["bmp"] = DirectContent("image/bmp"),
	["mp3"] = DirectContent("audio/mpeg"),
	["ogg"] = DirectContent("audio/ogg"),
	["mid"] = DirectContent("audio/mid"),
	["tif"] = DirectContent("image/tiff"),
	["tiff"] = DirectContent("image/tiff"),
	["eot"] = DirectContent("application/vnd.ms-fontobject"),
	["svg"] = DirectContent("image/svg+xml"),
	["ttf"] = DirectContent("application/x-font-truetype"),
	["woff"] = DirectContent("application/x-font-woff"),
	["woff2"] = DirectContent("application/x-font-woff2"),
}

function DataLive.New(dispatcher : WorkDispatcher, ip : string, directory : string) : Terminal
	return Terminal.New(dispatcher, ip, function (terminal : Terminal, uri : string, method : string, header : { string : string }, data : string) : (number, string, { string : string }, string)
		if string.sub(uri, -1) == "/" then
			uri = uri .. "index.html"
		end

		-- find ext
		local ext = uri:match("^.+%.(.+)$")
		local handler = translators[ext]
		
		-- parse selected lua
		print("New request [" .. method .. "] = " .. uri)
		if handler then
			local filePath = directory .. uri
			local content = SnowyStream.FetchFileData(filePath) as string
			if content and #content ~= 0 then
				local request : Request = {
					Code = 200,
					Method = method,
					InputHeader = header,
					InputData = data,
					OutputHeader = {},
					OutputData = "",
					Reason = "OK"
				}

				handler(content, request, filePath)
				return request.Code, request.Reason, request.OutputHeader, request.OutputData
			else
				print("File not found: " .. filePath)
			end
		end

		return 404, "Page not found", {}, ""
	end, function (connection : Connection, isConnect : boolean, src : string, dst : string) : boolean
		return true
	end, true)
end

return DataLive
