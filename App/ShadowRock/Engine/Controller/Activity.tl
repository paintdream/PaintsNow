local predefined = {
	Activate = "EVENT_ENTITY_ACTIVATE",
	Deactivate = "EVENT_ENTITY_DEACTIVATE",
	Tick = "EVENT_TICK",
	PreTick = "EVENT_PRETICK",
	PostTick = "EVENT_POSTTICK",
	AttachComponent = "EVENT_ATTACH_COMPONENT",
	DetachComponent = "EVENT_DETACH_COMPONENT",
	Frame = "EVENT_FRAME",
	FrameSyncTick = "EVENT_FRAME_SYNC_TICK",
	Input = "EVENT_INPUT",
	Output = "EVENT_OUTPUT"
}

local setuservalue = debug.setuservalue

return function (name : string, prototype : { string : any }) : Component
	local scriptComponent = ScriptComponentModule.New(name)
	for k, v in pairs(getmetatable(prototype) as { string : any }) do
		local slot = predefined[k]
		if slot then
			v = rawget(prototype, k) or v -- allow overriding
			scriptComponent:SetHandler(slot, function (... : any)
				v as function (... : any)(prototype, ...)	
			end)
		end
	end

	setuservalue(scriptComponent, prototype)
	return scriptComponent as Component
end