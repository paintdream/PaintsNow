-- derive from entity

local Scene = record
	spaceComponent : SpaceComponent
	rootEntity : Entity
	__index : any
end

Scene.__index = Scene

local Core = require("Engine/Core")
local Database = require("Engine/Model/Database")

function Scene.New(database : Database, clock : Clock) : Scene
	local meta = {} as Scene
	local rootEntity = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, false)
	rootEntity:AddEntityComponent(spaceComponent as Component)
	meta.spaceComponent = spaceComponent
	meta.rootEntity = rootEntity

	local RuntimeVisibilityProbe = require("Script/Prefabs/Components/Visibility/RuntimeVisibilityProbe")
	local eventComponent = EventComponentModule.New()
	eventComponent:BindEventTick(clock)
	eventComponent:BindEventFrame(true)
	rootEntity:AddEntityComponent(eventComponent as Component)

	local renderPolicy = ModelComponentModule.NewRenderPolicy("Primitives", 0, 1)
	local lightPolicy = ModelComponentModule.NewRenderPolicy("LightSource", 0, 1)
	local directRenderPolicy = ModelComponentModule.NewRenderPolicy("DirectPrimitives", 0, 1)

	local CustomShader = require("Script/Prefabs/Components/Shader/CustomShader")
	local StandardShader = require("Script/Shaders/Standard") as ShaderDescription
	local customShaderComponent = CustomShader.New(StandardShader)

	-- attach to an entity
	rootEntity:AddEntityComponent(customShaderComponent as Component)
	local batchComponent = BatchComponentModule.New("")
	rootEntity:AddEntityComponent(batchComponent as Component)
	
	customShaderComponent:SetComplete(function (info : string, code : string)
		if info and #info ~= 0 then
			print("[CustomShader.New] Compiling shader: " .. StandardShader.Name .. " failed!")
			print("[CustomShader.New] Error: " .. info)
		else
			print("[CustomShader.New] Compiling shader: " .. StandardShader.Name)
		end

		print("[CustomShader.New] Shader generated: ")
		print(code)
	end)

	-- add scene entities
	if database then
		print("\tWarming up resources ...")
		local warmingUpResources : { string : { string }} = {}
		for _, line in database:Lines("select Path, Names.Name from `Resources`, `Names` where `TypeID` == Names.ID", 0, nil as HoneyData) do
			local path, name = table.unpack(line as { string })
			warmingUpResources[name as string] = warmingUpResources[name as string] or {}
			table.insert(warmingUpResources[name as string], path as string)
		end

		local running = coroutine.running()
		local resourceHandles : { ResourceBase } = {}
		for name, list in pairs(warmingUpResources) do
			print("\tWarming up " .. name)
			SnowyStream.NewResourcesAsync(list, name, function (path : string, res : ResourceBase)
				--print("\t\tLoaded: " .. path)
				table.insert(resourceHandles, res)
			end, function () coroutine.resume(running) end)

			coroutine.yield()
		end

		-- wait for all resource scheduled
		print("\tSynchronizing resource creation ...")
		rootEntity:WaitForNextFrame(function () coroutine.resume(running) end)
		coroutine.yield() -- also switch to warp of rootEntity

		-- all resources prepared.

		local dupCount = 6
		print("\tCreating entities ...")

		local events : { Event } = {}
		for i = 1, dupCount do
			table.insert(events, Core.Event())
		end

		local skyTexture = nil as TextureResource

		for i = 1, dupCount do
			print("\tCreating at warp " .. i)
			local offset = (i - 1) * 7.5
			local interSpaceComponent = SpaceComponentModule.New(i, false)
			local index = i
			rootEntity:AddEntityComponent(interSpaceComponent as Component)

			BridgeSunset.QueueRoutine(interSpaceComponent, function ()
				local holdWarmingResources = resourceHandles -- in case of gc
				local subRootEntity = MythForest.NewEntity(-1)
				local subSpaceComponent = SpaceComponentModule.New(-1, true)
				local subSpaceComponentHuge = SpaceComponentModule.New(-1, false)
				local subTransformComponent = TransformComponentModule.New()
				subTransformComponent:SetTranslation({ offset, 0, 0 })
				subTransformComponent:UpdateTransform()
				subRootEntity:AddEntityComponent(subTransformComponent as Component)
				subRootEntity:AddEntityComponent(subSpaceComponent as Component)
				subRootEntity:AddEntityComponent(subSpaceComponentHuge as Component)

				local entities : { Entity } = {}
				local hugeEntity : { Entity : boolean } = {}
				local hugeComponent : { Component : boolean } = {}

				for _, line in database:Lines("select ID from `Entities`;", 32, nil as HoneyData) do
					local entityID = table.unpack(line as { number }) as number
					entities[entityID] = MythForest.NewEntity(-1)
				end

				print("\tCreating model components.")
				local components : { Component } = {}
				local shapeComponents : { ShapeComponent } = {}
				-- local formComponents : { FormComponent } = {}
				for _, line in database:Lines("select ModelComponents.ID, Path, ViewDistance from `ModelComponents`, `Resources` where `ResourceID` == Resources.ID;", 32, nil as HoneyData) do
					local componentID, path, viewDistance = table.unpack(line as { any })
					if not path as string:find("Sky") then
						local meshResource = SnowyStream.NewResource(path as string, "MeshResource", false) as MeshResource
						if not meshResource then
							print("\tUnable to load mesh resource: " .. path as string)
						else
							if not components[componentID as number] then
								local modelComponent = ModelComponentModule.New(meshResource, batchComponent) as ModelComponent
								modelComponent:AddRenderPolicy(renderPolicy)
								components[componentID as number] = modelComponent as Component
							end

							shapeComponents[componentID as number] = shapeComponents[componentID as number] or ShapeComponentModule.New(meshResource) 
							-- formComponents[componentID as number] = formComponents[componentID as number] or FormComponentModule.New(path as string)
						end
					end
				end

				local count = 0
				for _, line in database:Lines("select ModelComponentMaterials.ID, MeshGroupID, Path from `ModelComponentMaterials`, `Resources` where `ResourceID` == Resources.ID;", 32, nil as HoneyData) do
					local componentID, meshGroupID, path = table.unpack(line as { any })
					local materialResource = SnowyStream.NewResource(path as string, "MaterialResource", false) as MaterialResource
					if not materialResource then
						print("\tUnable to load material resource: " .. path as string)
					else
						local modelComponent = components[componentID as number] as ModelComponent
						if modelComponent then
							-- override material
							materialResource = customShaderComponent:ExportMaterial(materialResource)
							modelComponent:SetMaterial(meshGroupID as number, 0, materialResource)
						end
					end

					count = count + 1
				end

				print("\tCreating transform components.")
				for _, line in database:Lines("select ID, X, Y, Z, Scale, Yaw, Pitch, Roll from `TransformComponents`;", 32, nil as HoneyData) do
					local componentID, x, y, z, s, yaw, pitch, roll = table.unpack(line as { number })
					local transformComponent = TransformComponentModule.New()
					transformComponent:SetTranslation({ x as number, y as number, z as number })
					transformComponent:SetScale({ s as number, s as number, s as number })
					transformComponent:SetRotation({ yaw as number, pitch as number, roll as number })
					transformComponent:UpdateTransform()

					components[componentID as number] = transformComponent as Component
				end

				print("\tCreating envcube components")
				for _, line in database:Lines("select EnvCubeComponents.ID, Path from `EnvCubeComponents`, `Resources` where `ResourceID` == Resources.ID;", 32, nil as HoneyData) do
					local componentID, path = table.unpack(line as { any })
					components[componentID as number] = components[componentID as number] or EnvCubeComponentModule.New() as Component
					local textureResource = SnowyStream.NewResource(path as string, "TextureResource", false)
					if textureResource then
						local component = components[componentID as number] as EnvCubeComponent
						hugeComponent[component as Component] = true
						component:SetTexture(textureResource as TextureResource)
						component:AddRenderPolicy(lightPolicy)
						component:SetRange({1000, 1000, 1000})

						if not skyTexture then
							skyTexture = textureResource as TextureResource
						end
					else
						print("\tUnable to load texture resource: " .. path as string)
					end
				end

				print("\tAttaching components.")
				-- create model components
				for _, line in database:Lines("select ID, ComponentID from `EntityComponents`;", 32, nil as HoneyData) do
					local entityID, componentID = table.unpack(line as { any })
					local entity = entities[entityID as number]
					local component = components[componentID as number]
					if entity and component then
						if hugeComponent[component] then
							hugeEntity[entity] = true
						end
						entity:AddEntityComponent(component)
						local shapeComponent = shapeComponents[componentID as number]
						if shapeComponent then
							entity:AddEntityComponent(shapeComponent as Component)
						end

						--[[
						local formComponent = formComponents[componentID as number]
						if formComponent then
							entity:AddEntityComponent(formComponent as Component)
						end
						]]
					end
				end

				for _, entity in ipairs(entities) do
					entity:UpdateEntity(true)
					if hugeEntity[entity] then
						subSpaceComponentHuge:InsertEntity(entity)
					else
						subSpaceComponent:InsertEntity(entity)
					end
				end

				subSpaceComponent:Optimize()

				local subEventComponent = EventComponentModule.New()
				subEventComponent:BindEventFrame(true)
				subRootEntity:AddEntityComponent(subEventComponent as Component)
				interSpaceComponent:InsertEntity(subRootEntity)
				BridgeSunset.Pin(interSpaceComponent) -- all changes applied, turn it to static

				Core.Signal(events[index])
			end)
		end

		print("\tWaiting for components loading ... ")
		for i = 1, dupCount do
			Core.Wait(events[i])
		end

		-- switch to rootEntity's warp
		local event = Core.Event()

		BridgeSunset.QueueRoutine(rootEntity, function ()
			Core.Signal(event)
		end)

		Core.Wait(event)

		local visibilityComponent = RuntimeVisibilityProbe.New()
		rootEntity:AddEntityComponent(visibilityComponent as Component)

		print("\tAdding lights")
		local staticSpaceComponent = SpaceComponentModule.New(-1, true)
		rootEntity:AddEntityComponent(staticSpaceComponent as Component)

		local sun = MythForest.NewEntity(-(1))
		local sunTransformComponent : TransformComponent = TransformComponentModule.New()
		sunTransformComponent:SetScale({ 8, 8, 8 })
		sunTransformComponent:SetRotation({0.20 * math.pi, 0.85 * math.pi, 0})
		sunTransformComponent:SetTranslation({-3, 2, 4})
		sunTransformComponent:UpdateTransform()
		sun:AddEntityComponent(sunTransformComponent as Component)
		local lightComponent = LightComponentModule.New()
		lightComponent:SetLightColor({10, 10, 10})
		lightComponent:SetLightRange({1000, 1000, 1000})
		lightComponent:SetLightAttenuation(0.1)
		lightComponent:SetLightDirectional(true)
		lightComponent:AddRenderPolicy(lightPolicy)
		local streamComponent0 = StreamComponentModule.New({4, 4, 4}, 4)
		lightComponent:BindLightShadowStream(0, streamComponent0, { 1024, 1024 }, 0.1, 0.8)
		local streamComponent1 = StreamComponentModule.New({4, 4, 4}, 3)
		lightComponent:BindLightShadowStream(1, streamComponent1, { 1024, 1024 }, 0.5, 1.5)
		local streamComponent2 = StreamComponentModule.New({4, 4, 4}, 3)
		lightComponent:BindLightShadowStream(2, streamComponent2, { 1024, 1024 }, 4, 5)

		sun:AddEntityComponent(lightComponent as Component)
		staticSpaceComponent:InsertEntity(sun)

		if skyTexture then
			local skyComponent = SkyComponentModule.New(nil as MeshResource, nil as MaterialResource, batchComponent)
			skyComponent:AddRenderPolicy(directRenderPolicy)
			skyComponent:SetSkyTexture(skyTexture)
			local skyEntity = MythForest.NewEntity(-1)
			local skyTransform = TransformComponentModule.New()
			skyTransform:SetScale({ 500, 500, 500})
			skyTransform:SetDynamic(true)
			skyTransform:UpdateTransform()
			skyEntity:AddEntityComponent(skyTransform as Component)
			skyEntity:AddEntityComponent(skyComponent as Component)
			skyEntity:UpdateEntity(false)

			staticSpaceComponent:InsertEntity(skyEntity)
		end

		print("\tAdding point lights")
		math.randomseed(20)
		local colors : { {number} } = {
			{1, 0, 0},{0, 1, 0}, {0, 0, 1}, {1, 1, 0}, {0, 1, 1}, {1, 0, 1}, {1, 1, 1}, {2, 2, 2}
		}

		local radius : { {number} } = {}

		for k = 1, 8 do 
			local c = colors[k]
			local r = math.random(20, 40)
			for j = 1, 3 do
				c[j] = c[j] * r
			end

			table.insert(radius, { r / 12, r / 12, r / 12 })
		end
		for i = 1, 32 do 
			local light = MythForest.NewEntity(-(((1))))
			local lightTransformComponent = TransformComponentModule.New()
			lightTransformComponent:SetRotation({0.25 * math.pi, 0.75 * math.pi, 0})
			lightTransformComponent:SetTranslation({math.random(0, 15), math.random(2,4), math.random(3,6)})
			lightTransformComponent:UpdateTransform()
			light:AddEntityComponent(lightTransformComponent as Component)
			local lightComponent = LightComponentModule.New()
			lightComponent:SetLightColor(colors[((i + 1) % 8) + 1])
			lightComponent:SetLightRange(radius[((i + 1) % 8) + 1])
			lightComponent:SetLightAttenuation(math.random(100,400) * 0.2)
			lightComponent:SetLightDirectional(false)
			lightComponent:AddRenderPolicy(lightPolicy)
			light:AddEntityComponent(lightComponent as Component)
			staticSpaceComponent:InsertEntity(light)
		end

		-- BridgeSunset.Pin(staticSpaceComponent)
	end

	return setmetatable(meta, Scene as METATABLE) as Scene
end

return Scene