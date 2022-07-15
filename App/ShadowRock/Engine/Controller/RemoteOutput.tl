local Timeline = require("Engine/Model/Timeline")
local RemoteOutput = record
	__index : any
end

RemoteOutput.__index = RemoteOutput

return RemoteOutput