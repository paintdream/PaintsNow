local runtimeexts = { "", ".lua" }
local exts = runtimeexts
TypedDescriptions = {}

if EnableTL then
	print("Overriding io.open/close")
	exts = { "", ".tl", ".lua" }
	io = io or {}
	local orgOpen = io.open
	local orgClose = io.close
	local orgSearcher = package.searchpath

	local simfile = {}
	simfile.__index = simfile

	function simfile:read(s)
		local c = self.content
		self.content = nil
		return c
	end

	function simfile:close()
	end

	-- io operations only for tld usage
	io.open = function (f, m)
		-- print("Opening file " .. f)	
		local desc = TypedDescriptions[f]
		if type(desc) == "string" then
			local file = { content = desc }
			-- print("Extern Module " .. f)
			setmetatable(file, simfile)
			return file
		end

		-- override all tld requests
		-- print("Try Load resource: " .. f)
		local content = SnowyStream.FetchFileData(f)
		if content and #content ~= 0 then
			local file = { content = content }
			-- print("Buildin Module " .. f)
			setmetatable(file, simfile)
			return file
		end

		return nil, "Unable to find or empty content: " .. f
	end

	io.close = function (f)
		return type(f) ~= "table" and oldclose(f)
	end

	os = {}
	os.getenv = function() return end

	tl = nil
end

function package.searchpath(name, filter, ...)
	if TypedDescriptions[name] then
		return nil, "No tld file for Buildin module " .. name
	end

	for i, v in ipairs(LoadLuaFirst and runtimeexts or exts) do
		local c = name .. v
		if SnowyStream.FileExists(c) then
			return c
		end
	end

	return nil, "Failed to open file: " .. name	
end

function require(name, ...)
	local mod = package.loaded[name]
	if mod then
		return mod
	end

	-- print("Require " .. name)

	path, msg = package.searchpath(name, "")
	if not path then
		print("Module " .. name .. " not found. Message: " .. msg)
		return nil
	end

	local content = SnowyStream.FetchFileData(path)
	if content then
		local chunk, errMsg = load(content, name, "t", _ENV)
		if chunk then
			mod = chunk(name)
		else
			print("Load module " .. name .. " failed!")
			print("Compiler log: " .. errMsg)
		end
	end

	package.loaded[name] = mod
	return mod
end

function hotreload(forceRecompile)
	LoadLuaFirst = false
	local QuickCompile = require("Engine/Boot/QuickCompile")
	local compiler = QuickCompile.New()

	print("Hot compiling ...")
	local changedFiles = {
		compiler:CompileRecursive("Engine/", forceRecompile),
		compiler:CompileRecursive("Script/", forceRecompile)
	}

	print("Hot reloading ...")
	LoadLuaFirst = true
	for i = 1, 2 do
		for _, files in ipairs(changedFiles) do
			if files.__ordered then
				for _, name in pairs(files.__ordered) do
					if files[name] or forceRecompile then
						if i == 1 then
							package.loaded[name] = nil -- unload
						else
							require(name)
						end
					end
				end
			end
		end
	end

	LoadLuaFirst = false

	print("Hot reload finished ...")
end

if EnableTL then
	print("Initializing tl ...")
	tl = require("tl")

	-- register API prototypes
	local mapTypes = {
		--["char"] = "number",
		["int"] = "number",
		["short"] = "number",
		["float"] = "number",
		["double"] = "number",
		--["__int64"] = "number",
		["__int64"] = "number",
		["__ptr64"] = "number",
		--["long"] = "number",
		["long"] = "number",
		["string"] = "string",
		["String"] = "string",
		["bool"] = "boolean",
		["vector"] = "{any}",
		["BaseDelegate"] = "any",
		["Ref"] = "any",
		["Arguments"] = "...:any",
		["WarpTiny"] = "any",
		["void"] = "any",
	}

	function Bootstrap.GetTypeName(t, regTypes)
		if type(t) == "string" then
			local m = mapTypes[t]
			assert(m, "Unrecognized type: " .. t)
			return m
		end

		local m = mapTypes[t.Type]
		if m then return m end

		local r = regTypes[t.Type]
		if not r then
			local stmt = {}
			if not t.Pointer then
				table.insert(stmt, "global " .. t.Type .. " = record" .. "\n")
				regTypes[t.Type] = true
				local mid = {}
				for k, v in SortedPairs(t.Fields) do
					if type(k) == "string" then
						table.insert(mid, "\t" .. k .. ": " .. Bootstrap.GetTypeName(v, regTypes))	
					else
						--table.insert(mid, "\t" .. Bootstrap.GetTypeName(v, regTypes))	
						table.insert(mid, "\t{ " .. Bootstrap.GetTypeName(v, regTypes) .. " }")	
						break
					end
				end
				table.insert(stmt, table.concat(mid, "\n"))
				-- table.insert(stmt, "\t\"__rawpointer\" : number")
				table.insert(stmt, "\nend\n")

				regTypes[t.Type] = table.concat(stmt)
			end
		end

		return (t.List and "{ number : " or "") .. t.Type .. (t.List and " }" or "")
	end
end