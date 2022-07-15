local Core = require("Engine/Core")

local Timeline = record
	component : TapeComponent
	isReplay : boolean
	pushHead : { string : any }
	popHead : { string : any }
end

function Timeline:SetReplay(enable : boolean)
	assert(not enable or (self.component ~= nil as TapeComponent))
	self.isReplay = enable
end

function Timeline:Flush()
	self.component:Flush(nil)
end

function Timeline:Push(seq : number, event : { string : any })
	-- blocks all pushes on replaying
	if self.isReplay then
		return
	end

	-- record event
	if self.component then
		self.component:Write(seq, Core.Encode(event, nil as { any : any }))
	end

	-- chain to timeline
	event.__next = self.pushHead
	event.__seq = seq

	self.pushHead = event
	self.popHead = self.popHead or event
end

function Timeline:Pop() : (number, { string : any })
	if self.isReplay then
		-- read from component
		local info = self.component:Read()
		return info[1] as number, (Core.Decode(info[2] as string, nil as { any })) as { string : any } -- seq, event
	else
		local event = self.popHead
		if event then
			self.popHead = self.popHead.__next as { string : any }
			local seq = event.__seq as number
			event.__seq = nil
			event.__next = nil

			return seq, event
		end
	end
end

function Timeline:Seek(seq : number)
	assert(self.component)
	self.component:Seek(seq)
end

function Timeline.New(tapePath : string, isReplay : boolean) : Timeline
	local component : TapeComponent
	if tapePath then
		local res = SnowyStream.NewResource(tapePath, "StreamResource", true)
		assert(res, "Unable to new resource: " .. tapePath)
		component = TapeComponentModule.New(res as SharedTiny, 4096)
	end

	local object = {
		component = component
		isReplay = false,
		pushHead = nil,
		popHead = nil,
		isReplay = isReplay
	}

	return setmetatable(object, TapeComponent as METATABLE) as Timeline
end

return Timeline