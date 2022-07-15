local Activity = require("Engine/Controller/Activity")
local Guardian = record
	component : Component
end

function Guardian:Activate()
	print("Guardian awake.")
end

function Guardian:Deactivate()
	print("Guardian sleep.")
end

function Guardian.New() : Guardian
	local object = setmetatable({}, Guardian as METATABLE) as Guardian
	object.component = Activity("Guardian", object)
	return object
end

return Guardian