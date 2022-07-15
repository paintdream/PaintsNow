local Timeline = require("Engine/Model/Timeline")
local EventDispatcher = record
	__index : any
end

EventDispatcher.__index = EventDispatcher

return EventDispatcher