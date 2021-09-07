local ScriptDispatcher = record
	WarpBinding : { number }
	ScriptPool : { RemoteComponent }
	ManagedWarp : boolean
	__index : any
end

ScriptDispatcher.__index = ScriptDispatcher

function ScriptDispatcher.New(warpBinding) : ScriptDispatcher
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
	for i, warpIndex in ipairs(warps) do
		local remoteComponent = RemoteComponentModule.New(true, warpIndex) -- use transparent mode
		table.insert(scriptPool, remoteComponent)
	end

	return setmetatable({
		WarpBinding = warps,
		ScriptPool = scriptPool,
		ManagedWarp = managedWarp
	}, ScriptDispatcher as METATABLE)
end

function ScriptDispatcher.Dispatch(func : function (... : any) : any...)
	-- TODO:
end

return ScriptDispatcher