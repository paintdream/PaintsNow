local methodMap = {}
local dependMap = {}
local Inspect = PurpleTrail.Inspect
local function CollectIndexersRecursive(modname, collection)
	if type(collection) == "table" then
		if collection.__delegate__ then

			for name, desc in SortedPairs(Inspect(collection.__delegate__)) do
				if type(desc) == "table" then
					local mainName
					local function Register(params)
						for i, param in ipairs(params) do
							if type(param) == "table" and param.Type:find("PaintsNow::") then
								if i == 1 then
									mainName = param.Type
									methodMap[mainName] = methodMap[mainName] or {}
									methodMap[mainName][name] = collection[name]
								elseif mainName and param.Type ~= mainName then
									dependMap[mainName] = dependMap[mainName] or {}
									dependMap[mainName][param.Type] = true
								end
							end
						end
					end

					Register(desc.Params)
					Register(desc.Returns)
				end
			end
		end

		for k, v in pairs(collection) do
			CollectIndexersRecursive(k, v)
		end
	end
end

CollectIndexersRecursive("System", System)

local topSorted = {}
local visited = {}
for k, v in pairs(dependMap) do
	for j in pairs(v) do
		methodMap[j] = methodMap[j] or {}
	end
end

while true do
	local b = true
	for name, indexMap in SortedPairs(methodMap) do
		if not visited[name] and (dependMap[name] == nil or next(dependMap[name]) == nil) then
			table.insert(topSorted, { name = name, indexer = indexMap })
			for k, v in pairs(dependMap) do
				v[name] = nil
			end

			b = false
			visited[name] = true
		end
	end

	if b then
		break
	end
end

for k, v in pairs(dependMap) do
	if next(v) ~= nil then
		print("Depend class of " .. k .. " cycled.")
		local s = ""
		for t in pairs(v) do
			s = s .. "| " .. t
		end
		print("With " .. s)
	end
end

for i, v in ipairs(topSorted) do
	setindexer(v.name, v.indexer)
end

if EnableTL then
	local function SimplyName(t)
		local prefix = t:find(":[^:]*$")
		if prefix then
			return t:sub(prefix + 1)
		else
			return t
		end
	end

	print("Generating runtime files ...")
	local GetTypeName = Bootstrap.GetTypeName
	local methodTldMap = {}

	local function RegisterModule(regTypes, mod, collection)
		local tld = {}
		table.insert(tld, "global " .. mod .. " = record")
		for name, desc in SortedPairs(Inspect(collection.__delegate__)) do
			if type(desc) == "table" then
				local paramsList = {}
				local returnList = {}
				local firstParam = desc.Params[1]
				local typeName = type(firstParam) == "table" and firstParam.Type or ""
				for index, t in ipairs(desc.Params) do
					t.Type = SimplyName(t.Type)
					local name = GetTypeName(t, regTypes)
					table.insert(paramsList, name)
				end

				for index, t in ipairs(desc.Returns) do
					t.Type = SimplyName(t.Type)
					local name = GetTypeName(t, regTypes)
					table.insert(returnList, name)
				end

				local declare = "\t" .. name .. ": function (" .. table.concat(paramsList, ", ") .. ") : (" .. table.concat(returnList, ", ") .. ")"
				table.insert(tld, declare)
				if typeName:find("PaintsNow::") and name ~= "New" then
					local selfDeclare = "\t" .. name .. ": function (" .. table.concat(paramsList, ", ") .. ") : (" .. table.concat(returnList, ", ") .. ")"
					methodTldMap[typeName] = methodTldMap[typeName] or {}
					table.insert(methodTldMap[typeName], selfDeclare)
				end
			end
		end
		table.insert(tld, "end")

		return table.concat(tld, "\n")
	end

	print("TypedLua: Fetching module defs...")

	local tld = {}
	-- Preload
	local extramods = {}
	local regTypes = {}
	
	local function RegisterModulesRecursive(modname, collection)
		if type(collection) == "table" then
			if collection.__delegate__ then
				local tlddef = RegisterModule(regTypes, modname, collection)
				TypedDescriptions[modname] = tlddef
				table.insert(extramods, modname)
			end

			for k, v in pairs(collection) do
				RegisterModulesRecursive(k, v)
			end
		end
	end

	RegisterModulesRecursive("System", System)
	-- print("TypedLua: Defs loaded.")
	local interfaces = {}
	table.insert(interfaces, "global SharedTiny = record end\n")
	for name, val in SortedPairs(regTypes) do
		table.insert(interfaces, val)
	end

	for i, v in ipairs(topSorted) do
		table.insert(interfaces, "global " .. SimplyName(v.name) .. " = record")
		if methodTldMap[v.name] then
			table.insert(interfaces, table.concat(methodTldMap[v.name], "\n"))
		end
		table.insert(interfaces, "end\n")
	end

	-- add defination of interfaces
	TypedDescriptions["Interface"] = table.concat(interfaces, "\n")
	table.insert(extramods, "Interface")

	local fileHeader = [[
-- PaintsNow Script Interface Description File (TL Version)
-- This file is generated by LeavesWing in PaintsNow Suite. Please do not modify it manually.
--
]]
	local function WriteTldsRecurive(modname, collection)
		if type(collection) == "table" and TypedDescriptions[modname] then
			local tlddef = fileHeader .. TypedDescriptions[modname]
			local tldPath = "Runtime/".. modname .. ".tl"
			local content = SnowyStream.FetchFileData(tldPath)
			if not content or content ~= tlddef then
				print("Update runtime defs: " .. tldPath .. ". [" .. string.len(tlddef) .. " Bytes]")
				local file = SnowyStream.NewFile(tldPath, true)
				if file then
					SnowyStream.WriteFile(file, tlddef)
					SnowyStream.CloseFile(file)
				end
			end
			for k, v in pairs(collection) do
				-- WriteTldsRecurive(modname .. "." .. k, v)
				WriteTldsRecurive(k, v)
			end
		end
	end

	WriteTldsRecurive("System", System)
	WriteTldsRecurive("Interface", {})

	print("TypedLua: Type checking finished.")
end
