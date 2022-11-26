-- Engine/Core.lua
-- PaintDream (paintdream@paintdream.com)

package.path = package.path .. RootDirectory .. "/script/?.lua;" .. RootDirectory .. "/../script/?.lua;"
package.cpath = package.cpath .. RootDirectory .. "/script/?.dll;" .. RootDirectory .. "/../script/?.dll;"

Core = {}

function Core.Merge(g, t)
	for k, v in pairs(t) do
		if type(v) == "table" then
			local w = g[k] or {}
			assert(type(w) == "table")
			g[k] = Core.Merge(w, v)
		else
			g[k] = v
		end
	end

	return g
end

function Core.Diff(g, t)
	for k, v in pairs(t) do
		if type(t[k]) == "table" and type(v) == "table" then
			local w = g[k] or {}
			assert(type(w) == "table")
			g[k] = Core.Diff(w, v)
		else
			g[k] = nil
		end
	end

	return g
end

function Core.Patch(g, t)
	setmetatable(g, nil)

	for k, v in pairs(g) do
		g[k] = nil
	end

	for k, v in pairs(t) do
		g[k] = v
	end

	setmetatable(g, getmetatable(t))
	return g
end

function Core.UnHook(object)
	local mt = getmetatable(object)
	assert(mt)

	local org = getmetatable(mt)
	setmetatable(object, org)


	for k, v in pairs(mt.__Holder) do
		object[k] = v
	end

	return object
end

function Core.Hook(object, readHandler, writeHandler)
	local org = getmetatable(object)
	local holder = {}
	local mt = {}
	mt.__Holder = holder

	if readHandler then
		mt.__index = function(o, k)
			 assert(o == object)
			 return readHandler(object, k, (mt.__Holder)[k])
		end
	end

	if writeHandler then
		mt.__newindex = function(o, k, v)
			 assert(o == object)
			 if writeHandler then
				(mt.__Holder)[k] = writeHandler(object, k, (mt.__Holder)[k], v)
			 else
				(mt.__Holder)[k] = v
			 end
		end
	end


	for k, v in pairs(object) do
		(mt.__Holder)[k] = v
		object[k] = nil
	end

	setmetatable(mt, org)
	setmetatable(object, mt)
end

local function SafeStr(v)
	return (string.gsub(v, "'", "\\'"))
end

local function ToStr(v)
	local t = type(v)
	if t == "number" then
		return "" .. tostring(v)
	elseif t == "boolean" then
		return v and "true" or "false"
	elseif t == "string" then
		return "'" .. SafeStr(v) .. "'"
	end

	error("Error type " .. t)
	return "Invalid"
end

local function Safepairs(tab)
	local compare = {}
	for k in pairs(tab) do
		table.insert(compare, k)
	end

	table.sort(compare, function(a, b)
		local ta = type(a)
		local tb = type(b)
		return ta == tb and (a) < (b) or (ta) < (tb)
	end)
	local i = 0

	return function()
		i = i + 1
		local k = compare[i]
		local v = tab[k]
		return k, v
	end
end

function Core.Dump(obj, binary)
	local record = {}
	local str = {}

	table.insert(str, "return \n")

	local current = 1
	local function DumpEx(data)
		local t = type(data)
		if t == "table" then
			local node = data
			table.insert(str, "{\n")

			if record[node] then
				table.insert(str, "__ref = " .. tostring(record[node]))
			else
				record[node] = current
				current = current + 1
				for k, v in Safepairs(node) do
					 assert(type(k) == "boolean" or type(k) == "number" or type(k) == "string")
					 table.insert(str, "[" .. ToStr(k) .. "] = ")
					 DumpEx(v)
					 table.insert(str, ", \n")
				end
			end

			table.insert(str, "}")
		else
			local t = ToStr(data)
			if t then
				table.insert(str, t)
			end
		end
	end

	DumpEx(obj)

	if binary then
		local func = load(table.concat(str))
		return string.dump(func, true)
	else
		return str[1]
	end
end

local maskValueTable = 1
local maskValueRef = 2
local maskKeyTable = 4
local maskKeyRef = 8
local maskBoolAsNumber = 16

local function EncodeEx(object, formatDict, refDict)
	refDict = refDict or {}
	local ret = {}
	local format = {}
	table.insert(format, "!1<")

	local function AddDictEntry(object)
		refDict.__counter = (refDict.__counter or 0) + 1
		refDict[object] = refDict.__counter
	end

	AddDictEntry(object)

	local function WriteMask(typeMask, value, ref, tab)
		local typeValue = type(value)
		local cache = refDict[value]
		local retTypeMask = typeMask
		local retType = "I4"
		local retValue = ""

		if cache then
			retTypeMask = typeMask | ref
			retType = "I4"
			retValue = cache
		elseif typeValue == "table" then
			local s = EncodeEx(value, formatDict, refDict)
			cache = refDict[s]
			if cache then
				retTypeMask = typeMask | tab | ref
				retType = "I4"
				retValue = cache
			else
				AddDictEntry(s)
				retTypeMask = typeMask | tab
				retType = "s"
				retValue = s
			end
		elseif typeValue == "string" then
			AddDictEntry(value)
			retTypeMask = typeMask
			retType = "s"
			retValue = value
		elseif typeValue == "boolean" then
			retTypeMask = typeMask | maskBoolAsNumber
			retType = "I4"
			retValue = (value and 1 or 0)
		else
			assert(typeValue == "number", typeValue)
			retTypeMask = typeMask
			retType = "n"
			retValue = value
		end

		return retTypeMask, retType, retValue
	end

	for k, v in pairs(object) do
		table.insert(format, "b")
		local typeMask = 0
		local kt
		local kc
		local vt
		local vc

		typeMask, kt, kc = WriteMask(typeMask, k, maskKeyRef, maskKeyTable)
		typeMask, vt, vc = WriteMask(typeMask, v, maskValueRef, maskValueTable)

		table.insert(ret, typeMask)
		table.insert(format, kt)
		table.insert(ret, kc)
		table.insert(format, vt)
		table.insert(ret, vc)
	end

	local s = table.concat(format)
	local f = formatDict[s]
	if not f then
		formatDict.__counter = (formatDict.__counter or 0) + 1
		formatDict[s] = formatDict.__counter
		f = formatDict.__counter
	end

	local content = string.pack(s, table.unpack(ret))
	return string.pack("!1<I4s", f, content)
end

function Core.Encode(object, refDict)
	local formatDict = {}
	local data = EncodeEx(object, formatDict, refDict)
	local title = "!1<" .. string.rep("s", formatDict.__counter or 0)
	local formatTable = {}

	for i = 1, formatDict.__counter or 0 do
		formatTable[i] = false
	end

	for k, v in pairs(formatDict) do
		if k ~= "__counter" then
			formatTable[v] = k
		end
	end

	local format = string.pack(title, table.unpack(formatTable))
	return string.pack("!1<sss", title, format, data)
end

local function DecodeEx(data, formatDict, refDict)
	refDict = refDict or {}
	local s, content, unread = string.unpack("!1<I4s", data)
	local expand = table.pack(string.unpack(formatDict[s], content))
	local object = {}

	table.insert(refDict, object)

	local function WriteContent(object, typeMask, ref, tab)
		if (typeMask & ref) == ref then
			assert(type(object) == "number")
			local ret = refDict[object]
			if (typeMask & tab) == tab then
				ret = DecodeEx(ret, formatDict, refDict)
			end

			return ret
		elseif (typeMask & tab) == tab then
			assert(type(object) == "string")
			local ret = DecodeEx(object, formatDict, refDict)
			table.insert(refDict, object)
			return ret
		else
			if type(object) == "string" then
				table.insert(refDict, object)
			end

			return object
		end
	end

	for i = 1, #expand - 3, 3 do
		local typeMask = expand[i]
		local key = WriteContent(expand[i + 1], typeMask, maskKeyRef, maskKeyTable)
		local value = WriteContent(expand[i + 2], typeMask, maskValueRef, maskValueTable)

		if (typeMask & maskBoolAsNumber) == maskBoolAsNumber then
			value = (value ~= 0)
		end

		object[key] = value
	end

	return object, unread
end

function Core.Decode(data, refDict)
	local title, format, text = string.unpack("!1<sss", data)
	local formatTable = table.pack(string.unpack(title, format))
	return DecodeEx(text, formatTable, refDict)
end

function Core.Pipe()
	return {
		Coroutine = coroutine.running(),
		Waiting = false,
		Returns = {}
	}
end

function Core.Push(pipe, ...)
	if pipe.Waiting then
		coroutine.resume(pipe.Coroutine, ...)
	else
		pipe.Returns[#pipe.Returns + 1] = pipe.Returns[1]
		pipe.Returns[1] = table.pack(...)
	end
end

function Core.Pop(pipe)
	local returns
	if next(pipe.Returns) then
		returns = table.remove(pipe.Returns)
	else
		pipe.Waiting = true
		returns = table.pack(coroutine.yield())
		pipe.Waiting = false
	end
	
	return table.unpack(returns)
end

function Core.Invoke(func, bufferSize)
	local dispatcher = {}
	dispatcher.Buffer = {}
	dispatcher.BufferSize = bufferSize or ThreadCount
	dispatcher.Coroutine = coroutine.running()
	dispatcher.Count = 0
	dispatcher.WaitingComplete = false
	dispatcher.Completed = false

	local target = coroutine.create(func)

	Executive.Queue(function ()
		coroutine.resume(target, dispatcher)
	end)

	return dispatcher
end

function Core.Wait(dispatcher)
	dispatcher.WaitingComplete = true

	if not dispatcher.Completed then
		coroutine.yield()
	end

	dispatcher.WaitingComplete = false
	dispatcher.Completed = false
end

function Core.Dispatch(dispatcher, completion, routine, ...)
	-- must create from Core.Invoke in parent coroutine
	assert(dispatcher.Buffer)
	assert(dispatcher.Coroutine ~= coroutine.running())
	dispatcher.Count = dispatcher.Count + 1

	local dispatched = Executive.Dispatch(function (...)
		-- dispatch new if needed
		local buffer = dispatcher.Buffer

		while next(buffer) do
			dispatcher.Count = dispatcher.Count - 1
			if not Core.Dispatch(dispatcher, completion, table.unpack(table.remove(buffer))) then
				break
			end

			local barrier = dispatcher.BufferBarrier
			if barrier then
				dispatcher.BufferBarrier = nil
				-- resume wait marked by <----
				coroutine.resume(barrier)
			end
		end

		completion(...)
		dispatcher.Count = dispatcher.Count - 1

		if dispatcher.Count == 0 then
			dispatcher.Completed = true
			if dispatcher.WaitingComplete then
				coroutine.resume(dispatcher.Coroutine)
			end
		end
	end, routine, ...)
	
	if not dispatched then
		-- failed, try go deferred buffer
		local buffer = dispatcher.Buffer
		if #buffer >= dispatcher.BufferSize then
			assert(not dispatcher.BufferBarrier)
			dispatcher.BufferBarrier = coroutine.running()
			coroutine.yield() -- <---- wait for dispatch callback
		end
	
		assert(#buffer < dispatcher.BufferSize)
		table.insert(buffer, table.pack(routine, ...))
	end

	return dispatched
end

-- functions starts with '_' is called by Scrypter engine
-- shall NOT refer any upvalues in this function!
function _SetUpValues(func, upvalues)
	for i = 1, #upvalues / 2 do
		local index = upvalues[i * 2 - 1]
		local value = upvalues[i * 2]
		debug.setupvalue(func, index, value)
	end

	return func
end

function _EncodeRoutineEx(func)
	local binaryCode = string.dump(func)
	local upvalues = {}
	local nupvals = debug.getinfo(func, "u").nups

	for i = 1, nupvals do
		local name, value = debug.getupvalue(func, i)
		if name ~= "_ENV" and value ~= nil then
			table.insert(upvalues, i)
			table.insert(upvalues, value)
		end
	end

	return binaryCode, upvalues
end

function _EncodeRoutine(func)
	local binaryCode, upvalues = _EncodeRoutineEx(func)
	return binaryCode, Core.Encode(upvalues)
end

function _DecodeRoutine(binaryCode, upvalues)
	local func = load(binaryCode, "_Decoded")
	return _SetUpValues(func, Core.Decode(upvalues))
end

function _CompileRoutine(crossScripts, func)
	local crossRoutines = {}
	local binaryCode, upvalues = _EncodeRoutineEx(func)

	for i = 1, #crossScripts do
		local crossScript = crossScripts[i]
		local remoteFunc = CrossScriptModule.Load(crossScript, binaryCode)
		assert(remoteFunc)
		local setup = CrossScriptModule.Get(crossScript, "_SetUpValues")
		assert(setup)
		-- set upvalues
		CrossScriptModule.Call(crossScript, setup, remoteFunc, upvalues)
		table.insert(crossRoutines, remoteFunc)
	end

	return crossRoutines	
end

function print(...)
	return Executive.Log(table.concat({ ... }, "\t"))
end

function Core.WrapRoutine(routine)
	return function (...)
		local co = coroutine.create(routine)
		coroutine.resume(co, ...)
	end
end

function Core.WrapUploadParameters(config)
	return function (parameters)
		-- fill default values
		local map = {}
		for k, v in ipairs(config) do
			map[v.name]	= k
		end

		for i, v in ipairs(config) do
			config[v.name] = v.value
		end

		-- fill overrided values
		for i, v in ipairs(parameters) do
			config[v.name] = v.value
			config[map[v.name]].value = v.value
		end
	end
end

function Core.WrapDownloadParameters(config)
	return function ()
		return config
	end
end

return Core