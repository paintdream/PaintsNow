local Timeline = require("Engine/Model/Timeline")
local Event = require("Engine/Model/Event")
local Activity = require("Engine/Controller/Activity")
local LocalInput = record
	_timeline : Timeline
	_component : Component
	__index : any
end

LocalInput.__index = LocalInput

function LocalInput:Activate()

end

function LocalInput:Deactivate()

end

local EventKeyboard = record
	Name : string
	Down : boolean
end

local EventMouse = record
	Button : boolean
	Down : boolean
	Move : boolean
	Wheel : boolean
	Position : float2
end

local EventData = record
	Size : { number }
	Keyboard : EventKeyboard
	Mouse : EventMouse
end

function LocalInput:Input(event : EventData)
	-- interpret
end

function LocalInput.New(timeline : Timeline) : LocalInput
	local object = setmetatable({}, LocalInput as METATABLE) as LocalInput
	object._timeline = timeline
	object._component = Activity("LocalInput", object)
	
	return object
end

return LocalInput