-- DataHunter
-- A Scrypter demo for searching bytes in files with parallelized scripting
-- PaintDream (paintdream@paintdream.com)
--

local Config = {
	{ name = "Folder", description = "", type = "Directory", value = ""},
	{ name = "Text", description = "", type = "String", value = "" },
	{ name = "Extension", description = "", type = "String", value = "*.*" }
}

local function Main()
	local pipe = Core.Pipe()
	Core.Invoke(function (dispatcher)
		local RpcProcessFile = Executive.Compile(function (path, relativePath)
			local f = io.open(path, "rb")
			local content = f:read("*all")
			local offset = content:find(Config.Text)
			print("Searching: " .. path)

			if offset then
				Executive.Record(relativePath, "Offset: " .. tostring(offset), "")
			end

			f:close()
			return #content
		end)

		local function ProcessDirectory(path, relativePath)
			if path:find("/$") then
				for index, file in ipairs(Executive.QueryFiles(path, Config.Extension)) do
					ProcessDirectory(path .. file, relativePath .. file)
				end
			else
				Core.Dispatch(dispatcher, function (...)
					Core.Push(pipe, ...)	
				end, RpcProcessFile, path, relativePath)	
			end
		end

		ProcessDirectory(Config.Folder .. "/", "")

		-- print("Waiting for dispatcher!")
		Core.Wait(dispatcher)
		-- print("Ready to push end")
		Core.Push(pipe, nil)
	end)

	local totalBytes = 0
	local totalCount = 0
	while true do
		local bytes = Core.Pop(pipe)
		if bytes then
			-- print("BYTES: " .. tostring(bytes))
			totalBytes = totalBytes + bytes
			totalCount = totalCount + 1
		else
			-- print("BREAK")
			break
		end
	end
	
	print(string.format("DataHunter search completed. TotalBytes scanned: %d KB in %d files", math.ceil(totalBytes / 1024.0), totalCount))
	Executive.UpdateProfile("Complete", "", 1.0)
end

local function GetOperations()
	return {
		"Explore"
	}
end

local function DoExplore(object, description, cookie)
	local path = Config.Folder .. "/" .. object
	print("Explore: " .. path)
	Executive.ShellExecute("open", "explorer", "/select,\"" .. (path:gsub("/", "\\")) .. "\"", true)
end

local DispatchTable = {
	Main = Core.WrapRoutine(Main),
	DownloadParameters = Core.WrapDownloadParameters(Config),
	UploadParameters = Core.WrapUploadParameters(Config),
	GetOperations = GetOperations,
	Explore = DoExplore
}

-- print("DataHunter initialized!")

return function (func, ...)
	return DispatchTable[func](...)
end

