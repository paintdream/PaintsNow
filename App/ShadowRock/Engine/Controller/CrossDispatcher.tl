local DelayedRoutine = record
	Routines : { RemoteRoutine }
	Parameters : { any }
	Callback : any
end

local CrossDispatcher = record
	WarpBinding : { number }
	ScriptPool : { RemoteComponent }
	Setups : { RemoteRoutine }
	ManagedWarp : boolean
	DelayedRoutines : { DelayedRoutine }
	__index : any
end

global Awaitable = record
	Coroutine : thread
	Returns : { any }
end

CrossDispatcher.__index = CrossDispatcher

function CrossDispatcher:__gc()
	if self.ManagedWarp then
		for _, warpIndex in ipairs(self.WarpBinding) do
			BridgeSunset.FreeWarpIndex(warpIndex)
		end
	end
end

function CrossDispatcher.New(warpBinding : number | { number }) : CrossDispatcher
	local managedWarp = false
	local warps : { number } = {}
	if type(warpBinding) == "number" then
		-- allocate warp binding
		for i = 1, warpBinding do
			table.insert(warps, BridgeSunset.AllocateWarpIndex())
		end

		managedWarp = true
	else
		assert(type(warpBinding) == "table")
		warps = warpBinding as { number }
	end

	local scriptPool : { RemoteComponent } = {}
	local setups : { RemoteRoutine } = {}

	local setupEnvironment = [[
		local func, upvalues = ...
		for i = 1, #upvalues / 2 do
			local index = upvalues[i * 2 - 1]
			local value = upvalues[i * 2]
			debug.setupvalue(func, index, value)
		end
	]]

	for i, warpIndex in ipairs(warps) do
		local remoteComponent = RemoteComponentModule.New(true, warpIndex) -- use transparent mode
		table.insert(scriptPool, remoteComponent)
		table.insert(setups, remoteComponent:Load(setupEnvironment))
	end

	return setmetatable({
		WarpBinding = warps,
		ScriptPool = scriptPool,
		Setups = setups,
		ManagedWarp = managedWarp,
		DelayedRoutines = {}
	}, CrossDispatcher as METATABLE)
end

function CrossDispatcher:Setup(func : function (... : any) : any...) : { RemoteRoutine }
	local remoteRoutines : { RemoteRoutine } = {}
	local binaryCode = string.dump(func)
	local upvalues : { any } = {}
	local nupvals = debug.getinfo(func, "u").nups as number

	for i = 1, nupvals do
		local name, value = debug.getupvalue(func, i)
		if name ~= "_ENV" then
			table.insert(upvalues, i)
			table.insert(upvalues, value)
		end
	end

	for i, remoteComponent in ipairs(self.ScriptPool) do
		local remoteFunc = remoteComponent:Load(binaryCode)
		assert(remoteFunc)
		-- set upvalues
		remoteComponent:Call(self.Setups[i], remoteFunc, upvalues)
		table.insert(remoteRoutines, remoteFunc)
	end

	return remoteRoutines
end

function CrossDispatcher:_DispatchWithCallback(callback : any, remoteRoutines : { RemoteRoutine }, ... : any) : number
	local scriptPool = self.ScriptPool
	local count = #scriptPool

	for i = 1, count do
		local remoteComponent = scriptPool[i]
		if not remoteComponent:IsLocked() and remoteComponent:TryCallAsync(callback, remoteRoutines[i], ...) then
			return i
		end
	end

	return 0
end

function CrossDispatcher:PollDelayedRoutines()
	local routines = self.DelayedRoutines
	local count = #self.ScriptPool
	while next(routines) do
		local delayed = table.remove(routines)
		local id = self:_DispatchWithCallback(delayed.Callback, delayed.Routines, table.unpack(delayed.Parameters))
		if id == 0 then
			-- could not happen unless some RemoteComponent is being operated out of CrossDispatcher
			table.insert(routines, delayed)
			break
		elseif id == count then
			break
		end
	end
end

function CrossDispatcher:Dispatch(remoteRoutines : { RemoteRoutine }, ... : any) : Awaitable
	-- select proper component
	local awaitable = {} as Awaitable
	local callback = function (... : any)
		local co = awaitable.Coroutine
		if co then
			coroutine.resume(co, ...)
		else
			awaitable.Returns = table.pack(...) as { any }
		end
		
		-- poll next
		self:PollDelayedRoutines()
	end

	if self:_DispatchWithCallback(callback, remoteRoutines, ...) ~= 0 then
		return awaitable
	else
		-- failed, make delayed routine
		local delayed = {
			Routines = remoteRoutines,
			Parameters = table.pack(...) as { any }
			Callback = callback
		}

		table.insert(self.DelayedRoutines, delayed)
		return awaitable
	end
end

function CrossDispatcher:Wait(awaitable : Awaitable) : any...
	if awaitable.Returns then
		return table.unpack(awaitable.Returns)
	else
		awaitable.Coroutine = coroutine.running()
		return coroutine.yield()
	end
end

return CrossDispatcher