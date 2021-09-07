local CrossRoutine = record end

function CrossRoutine.New(callback : function (... : any)) : (function (func : function (... : any) : any...) : function (...:any) : any...)
	local remoteComponent = RemoteComponentModule.New(true, 0) -- use transparent mode
	local setupEnvironment = remoteComponent:Load([[
		local func, upvalues = ...
		for i = 1, #upvalues / 2 do
			local index = upvalues[i * 2 - 1]
			local value = upvalues[i * 2]
			debug.setupvalue(func, index, value)
		end
	]])

	return function (func : function (... : any) : any...) : function (...:any) : any...
		-- compile function 
		local nupvals = debug.getinfo(func, "u").nups as number
		local binaryCode = string.dump(func)
		local remoteFunc = remoteComponent:Load(binaryCode)
		assert(remoteFunc)
		-- set upvalues
		local upvalues : { any } = {}
		for i = 1, nupvals do
			local name, value = debug.getupvalue(func, i)
			if name ~= "_ENV" then
				table.insert(upvalues, i)
				table.insert(upvalues, value)
			end
		end

		remoteComponent:Call(setupEnvironment, remoteFunc, upvalues)

		if callback then
			return function (... : any) : any...
				remoteComponent:CallAsync(callback, remoteFunc, ...)
			end
		else
			return function (... : any) : any...
				local running = coroutine.running()
				local state = { done = false, waiting = false, rets = {} }
				remoteComponent:CallAsync(function (... : any)
					if state.waiting then
						coroutine.resume(running, ...)
					else
						state.done = true
						state.rets = table.pack(...)
					end
				end, remoteFunc, ...)

				state.waiting = true
				if state.done then
					return table.unpack(state.rets)
				else
					return coroutine.yield()
				end
			end
		end
	end
end

return CrossRoutine