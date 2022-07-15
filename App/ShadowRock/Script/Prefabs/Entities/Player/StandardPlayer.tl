local TpsCameraController = require("Script/Prefabs/Entities/CameraController/TpsCameraController")

local StandardPlayer = record
	cameraController : TpsCameraController
	entity : Entity
	__index : any
end

StandardPlayer.__index = StandardPlayer

function StandardPlayer.New(eventComponent : EventComponent, tpsCameraController : TpsCameraController) : StandardPlayer
	print("\tStardard Player selected.")
	local player = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, false)
	player:AddEntityComponent(spaceComponent as Component)

	print("\tAttach camera to player ...")
	spaceComponent:InsertEntity(tpsCameraController.entity)
	
	local meta = {} as StandardPlayer
	meta.cameraController = tpsCameraController
	meta.entity = player

	debug.setuservalue(player, meta)
	return setmetatable(meta, StandardPlayer as METATABLE) as StandardPlayer
end

return StandardPlayer