-- Main.lua
-- Application Entry Module
local Main = record end
local Debug = require("Script/Util/Debug")
local Core = require("Engine/Core")
local Database = require("Engine/Model/Database")
local Vector = require("Engine/Model/Math/Vector")
local DataLive = require("Engine/Controller/DataLive")

local fpsBuffer : { number } = {}
local tickBuffer : { number } = {}
local bufferIndex = 1

local function SetFrameMonitorOnTitle(deltaTime : number, cameraComponent : CameraComponent, transformComponent : TransformComponent, rayTraceComponent : RayTraceComponent)
	-- show fps
	fpsBuffer[bufferIndex] = MythForest.GetFrameTickDelta()
	tickBuffer[bufferIndex] = deltaTime

	bufferIndex = (bufferIndex + 1) % 8
	local sumFrame = 1
	local sumTick = 1

	for _, v in ipairs(fpsBuffer) do
		sumFrame = sumFrame + v
	end

	for _, v in ipairs(tickBuffer) do
		sumTick = sumTick + v
	end

	if bufferIndex % 4 == 0 then
		local position = transformComponent:GetTranslation()
		local trace = string.format("%3.2f%%", rayTraceComponent:GetCompletedPixelCount() * 100.0 / math.max(1, rayTraceComponent:GetTotalPixelCount()))
		local fps = string.format("%3.2f", math.min(1000 * #fpsBuffer / sumFrame, 999.99))
		local tps = string.format("%3.2f", math.min(1000 * #tickBuffer / sumTick, 999.99))
		local eye = string.format("%3.2f, %3.2f, %3.2f", position[1], position[2], position[3])
		local render = tostring(cameraComponent:GetCollectedDrawCallCount()) .. "/" .. tostring(cameraComponent:GetCollectedVisibleEntityCount()) .. "/" .. tostring(cameraComponent:GetCollectedEntityCount())
		local triangle = tostring(cameraComponent:GetCollectedTriangleCount())

		System.SetAppTitle("ShadowRock DEMO"
			.. " | Trace: " .. trace
			.. " | FPS: " .. fps
			.. " | TPS: " .. tps
			.. " | Eye: " .. eye
			.. " | Render " .. render
			.. " | Triangles " .. triangle)
	end
end

local function printbox(prefix : string, box : { { number } })
	print(prefix .. " BOX: "
	 .. tostring(box[1][1]) .. ", " .. tostring(box[1][2]) .. ", " .. tostring(box[1][3]) .. " - "
	 .. tostring(box[1][1]) .. ", " .. tostring(box[1][2]) .. ", " .. tostring(box[1][3])
	)
end

local function InitializeWorld(databasePath : string)
	SnowyStream.SetRenderResourceFrameStep(8)

	print("InitializeWorld: Loading repository " .. databasePath .. " ...")
	local database = Database.New(databasePath)
	if not database then
		print("No repostory found.")
		return
	end

	local clock : Clock = HeartVioliner.NewClock(1000.0 / 64, 0) -- 80 FPS logic by default
	print("InitializeWorld: Creating ticker ...")
	local eventComponent : EventComponent = EventComponentModule.New()
	eventComponent:BindEventTick(clock)
	eventComponent:BindEventFrame(true)
	eventComponent:BindEventUserInput(true)

	print("InitializeWorld: Creating GUI ...")
	local GUI = require("Script/Root/GUI")
	local gui = GUI.New()
	local guiEntity = gui.rootEntity

	print("\tCreating Camera ...")
	local TpsCameraController = require("Script/Prefabs/Entities/CameraController/TpsCameraController")
	local cameraController = TpsCameraController.New(eventComponent)
	-- set camera transform
	local cameraTranslation : float3 = { 5, 5, 8 }
	local cameraRotation : float3 = { 0.25 * math.pi, 0, math.pi }
	local cameraTransform = cameraController.inputTransformComponent
	cameraTransform:SetTranslation(cameraTranslation)
	cameraTransform:SetRotation(cameraRotation)
	cameraTransform:UpdateTransform()
	local cameraComponent = cameraController.cameraComponent
	print("\tCreating Player ...")
	local Player = require("Script/Prefabs/Entities/Player/StandardPlayer")
	local player = Player.New(eventComponent, cameraController)
	print("InitializeWorld: Mounting entities...")

	local screenSize = System.GetScreenSize() as { number }
	cameraController.aspect = screenSize[1] / screenSize[2]
	cameraController:UpdatePerspective()
	
	local lastMousePos = { 0, 0 }
	local moveSpeed = 1.0
	local inOperation = false

	local stepPhase = false
	local stepBounceCount : number = 16

	local function ReportStepConfig()
		print("PhaseStep: " .. (stepPhase and "[ON] " or "[OFF] BOUNCE: ") .. tostring(stepBounceCount))
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

	local scriptComponent = ScriptComponentModule.New("UserInputComponent")
	local phaseComponent : PhaseComponent
	cameraController.spaceComponent:InsertEntity(guiEntity)
	cameraController.entity:UpdateEntity(false)
	cameraController.entity:AddEntityComponent(scriptComponent as Component)

	print("InitializeWorld: Creating scene ...")
	local Scene = require("Script/Root/Scene")
	local scene = Scene.New(database, clock)
	local spaceComponent = scene.spaceComponent
	spaceComponent:InsertEntity(player.entity)
	cameraComponent:BindRootEntity(scene.rootEntity)

	-- raytrace compare
	local rayTraceComponent = RayTraceComponentModule.New()
	local rayTraceWidgetComponent = gui.rayTraceWidget
	cameraController.entity:AddEntityComponent(rayTraceComponent as Component)
	scene.rootEntity:UpdateEntity(true)

	local showRayTraceTexture = false
	local SwitchRayTraceTexture = function ()
		showRayTraceTexture = not showRayTraceTexture
		rayTraceWidgetComponent:SetVisible(showRayTraceTexture)
	end

	Debug.StartTrace = function (width : number, height : number, superSample : number, rayCount : number, bounceCount : number, averageLuminance : number, path : string)
		width = (width and width > 4) and width or 480
		height = (height and height > 4) and height or 270
		superSample = (superSample and superSample > 0) and superSample or 4
		rayCount = (rayCount and rayCount > 0) and rayCount or 16
		bounceCount = (bounceCount and bounceCount > 0) and bounceCount or 4
		averageLuminance = (averageLuminance and averageLuminance > 0) and averageLuminance or 1.0
		path = path or "./trace/example.png"

		BridgeSunset.QueueRoutine(rayTraceComponent, function ()
			rayTraceComponent:SetCaptureSize({width, height})
			rayTraceComponent:SetOutputPath(path)
			rayTraceComponent:Configure(superSample, 64, rayCount, bounceCount)

			rayTraceWidgetComponent:SetWidgetTexture(rayTraceComponent:GetCapturedTexture())
			showRayTraceTexture = false
			SwitchRayTraceTexture()
			rayTraceComponent:Capture(cameraComponent, averageLuminance)
		end)
	end

	local cacheInputs : { EventData } = {}
	local handleInput = function  (eventData : EventData)
		if inOperation then return end

		local evSize = eventData.Size
		if evSize then
			print("Resizing to (" .. tostring(evSize[1]) .. ", " .. tostring(evSize[2]) .. ")")
			cameraController.aspect = evSize[1] / evSize[2]
			cameraController:UpdatePerspective()
		end

		inOperation = true
		local key = eventData.Keyboard
		if key then
			-- print("KEY PRESSED: " .. key.Name)
			if key.Down then
				local name : string = string.upper(key.Name)
				if name == "W" then
					cameraController:Walk(moveSpeed)
				elseif name == "S" then
					cameraController:Walk(-moveSpeed)
				elseif name == "A" then
					cameraController:Translate({ -moveSpeed, 0 })
				elseif name == "D" then
					cameraController:Translate({ moveSpeed, 0})
				elseif name == '`' then
					SwitchRayTraceTexture()
				elseif name == "G" then
					-- step phase
					stepPhase = not stepPhase
					ReportStepConfig()
				elseif name == "," then
					stepBounceCount = math.min(stepBounceCount - 1, 1)
					ReportStepConfig()
				elseif name == "." then
					stepBounceCount = stepBounceCount + 1
					ReportStepConfig()
				end

				moveSpeed = math.min(moveSpeed + 0.1, 10.0)
			else
				moveSpeed = 1.0
			end
		end

		local mouse = eventData.Mouse
		if mouse then
			local size = System.GetScreenSize() as { number }
			local to = { mouse.Position[1] / size[1] * 2 - 1, mouse.Position[2] / size[2] * 2 - 1 }
			if mouse.Wheel then
				if mouse.Down then
					cameraController:Walk(moveSpeed)
				else
					cameraController:Walk(-moveSpeed)
				end
			elseif mouse.Down and mouse.Move then
				local from = { lastMousePos[1] / size[1] * 2 - 1, lastMousePos[2] / size[2] * 2 - 1 }

				if mouse.Button then
					-- Conflicts with LeavesWine.
					local s = moveSpeed * 100
					cameraController:Translate({ (from[1] - to[1]) * s, (from[2] - to[2]) * s} as float2)
				else
					cameraController:Rotate(from, to)
				end
			elseif mouse.Down then
				--[[
				local position = cameraTransform:GetTranslation() as { number }
				local mat = cameraTransform:GetAxises() as { { number }}
				local target = ((Vector.Attach(mat[3]) as number * (-1) + Vector.Attach(mat[1]) as number * to[1] * cameraController.aspect + Vector.Attach(mat[2]) as number * to[2]) * 100.0) as float3
				local RaycastResult = record
					Intersection : float3
					Normal : float3
					TexCoord : float2
					Distance : number
					Object : Component
					Parent : Entity
				end

				local results = scene.rootEntity:Raycast(function (results : { RaycastResult })
					for i, result in ipairs(results) do
						BridgeSunset.QueueRoutine(result.Parent, function ()
							local formComponent = result.Parent:GetUniqueEntityComponent("FormComponent") as FormComponent
							print("[" .. tostring(i) .. "] Mouse clicked at: " .. formComponent:GetName())
						end)
					end
				end, position, target, 1)
				]]
			end

			lastMousePos = mouse.Position
		end

		cameraController:Update()
		scene.rootEntity:UpdateEntity(true)
		inOperation = false
	end

	scriptComponent:SetHandler("EVENT_TICK", function (eventData : EventData)
		local deltaTime = eventData as number
		SetFrameMonitorOnTitle(deltaTime, cameraComponent, cameraTransform, rayTraceComponent)

		for _, input in ipairs(cacheInputs) do
			handleInput(input)
		end

		cacheInputs = {}

		if stepPhase and phaseComponent then
			phaseComponent:Step(stepBounceCount)
		end
	end)

	scriptComponent:SetHandler("EVENT_INPUT", function (eventData : EventData)
		table.insert(cacheInputs, eventData)
	end)

	local phaseComponent = cameraController.phaseComponent
	phaseComponent:BindRootEntity(scene.rootEntity)
	phaseComponent:SetDebugMode("./phases/")

	Debug.StartPhase = function ()
		BridgeSunset.QueueRoutine(phaseComponent, function ()
			phaseComponent:Update({ 4, 1.6, 5 })
		end)
	end

	print("InitializeWorld: World initialization finished. Waiting for system command")

	local instance = coroutine.running()
	System.ListenConsole(function (command : string)
		if command:sub(1, 1) == "@" then
			coroutine.resume(instance, command)
		else
			local func, message = load(command)
			if func then
				func()
			else
				print("Invalid command: " .. message as string)
			end
		end
	end)

	while true do
		local command = coroutine.yield()
		if command == "@Quit" then
			print("Battle control terminated.")
			eventComponent:BindEventTick(nil as Clock)
			eventComponent:BindEventUserInput(false)
			eventComponent:BindEventFrame(false)
			System.Exit()
			break
		end
	end
end

function Main.Main()
	print("Main initializing ...")
	print("Loading Repository Daemon ...")
	local databasePath = "Weaver/Repository.db"

	Debug.CompressResource = function (type : string)
		local database : Database = Database.New(databasePath)
		if database then
			local compressor = require("Script/Util/RepositoryCompressor")
			compressor.Compress(type, database)
		end
	end

	_G as { any : any } ["Debug"] = Debug
	local RepositoryDaemon = require("Script/Util/RepositoryDaemon")
	local weaver = RepositoryDaemon:New("127.0.0.1:16384", "Database/Repository.sql", databasePath)
	Debug.Weaver = weaver

	local workDispatcher : WorkDispatcher = EchoLegend.OpenDispatcher(false)
	EchoLegend.ActivateDispatcher(workDispatcher)
	local dataLive = DataLive.New(workDispatcher, "127.0.0.1:8080", "Web")

	Debug.Dispatcher = workDispatcher
	Debug.DataLive = dataLive
	
	local s, msg = pcall(function () : string
		InitializeWorld(databasePath)
		return "World Initialized"
	end)

	if not s then
		print("Error: " .. msg as string)
	end
end

return Main