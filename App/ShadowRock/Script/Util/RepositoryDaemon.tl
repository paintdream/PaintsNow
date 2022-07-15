local RepositoryDaemon = record
	__index : any
end

RepositoryDaemon.__index = RepositoryDaemon
local Database = require("Engine/Model/Database")

local function MakeDatabase(databasePath : string, sqlTemplatePath : string) : Database
	-- Always create a new database
	if databasePath ~= ":memory:" then
		SnowyStream.DeleteFile(databasePath)
	end

	local database = Database.New(databasePath)
	-- initialize table structure
	local schema = SnowyStream.FetchFileData(sqlTemplatePath) as string
	for sql in schema:gmatch("[^;]+;") do
		if sql then
			database:Execute(sql, nil as HoneyData)
		end
	end

	return database as Database
end

local function FilterPath(path : string) : string
	return (path:gsub(":", "/"))
end

local InspectResult = record
	Depends : { string }
end

function RepositoryDaemon:New(localAddress : string, sqlTemplatePath : string, databasePath : string) : Weaver
	-- Debug.TestDatabase = MakeDatabase(":memory:")
	-- database:Execute("Create table test(a int not null);", nil)
	-- mount debug objects
	local weaver = GalaxyWeaver.NewWeaver(localAddress)

	if weaver then
		local resources : { any } = {}
		local names : { { any } } = {}
		local nameMap : { string : number } = {}
		local resourceMap : { string : number } = {}
		local entities : { any } = {}
		local entityGroups : { any } = {}
		local components : { any } = {}
		local entityComponents : { any } = {}
		local modelComponents : { { any } } = {}
		local modelComponentMaterials : { { any } } = {}
		local envCubeComponents : { { any } } = {}
		local transformComponents : { { any } } = {}
		local spaceComponents : { { any } } = {}
		local cameraViewMatrix : { number } = {}
		local cameraFovNearFarAspect : float4 = { 90, 0.5, 2000, 1 }

		local function AllocateName(name : string, category : string) : number
			local idx = nameMap[name]
			if idx then
				return idx
			else
				local id = #names + 1
				table.insert(names, { id, category, name })
				nameMap[name] = id
				return id
			end
		end

		local handlers : { string : any } = {
			RpcCheckVersion = function (version : string)
				print("RepositoryDaemon: Client connected. Version = " .. version)
			end,
			RpcPostResource = function (path : string, ext : string)
				local typeID = AllocateName(ext, "Resource")
				path = FilterPath(path)
				print("RepositoryDaemon: Received resource " .. path .. "." .. ext)
				table.insert(resources, { #resources + 1, typeID, path })
				resourceMap[path .. "." .. ext] = #resources
			end,
			RpcPostEntity = function (entityID : number, groupID : number, name : string)
				table.insert(entities, { entityID, groupID, name })
			end,
			RpcPostEntityGroup = function (groupID : number, name : string)
				table.insert(entityGroups, { groupID, name })
			end,
			RpcPostEntityComponent = function (entityID : number, component : number)
				table.insert(entityComponents, { entityID, component })
			end,
			RpcPostModelComponent = function (componentID : number, resource : string, viewDistance : number)
				table.insert(components, { componentID, AllocateName("ModelComponent", "Component") })
				table.insert(modelComponents, { componentID, resource, viewDistance })
			end,
			RpcPostModelComponentMaterial = function (componentID : number, meshGroupID : number, resource : string)
				table.insert(modelComponentMaterials, { componentID, meshGroupID, resource })
			end,
			RpcPostTransformComponent = function (componentID : number, position : float3, scale : float3, rotation : float3)
				table.insert(components, { componentID, AllocateName("TransformComponent", "Component") })
				table.insert(transformComponents, { componentID, position[1], position[2], position[3], scale[1], rotation[1], rotation[2], rotation[3] })	
			end,
			RpcPostSpaceComponent = function (componentID : number, groupID : number)
				table.insert(components, { componentID, AllocateName("SpaceComponent", "Component") })
				table.insert(spaceComponents, { componentID, groupID })
			end,
			RpcPostEnvCubeComponent = function (componentID : number, texturePath : string)
				table.insert(components, { componentID, AllocateName("EnvCubeComponent", "Component")})
				table.insert(envCubeComponents, { componentID, texturePath })
			end,
			RpcUpdateView = function (viewMatrix : { number }, fovNearFarAspect : float4)
				cameraViewMatrix = viewMatrix
				cameraFovNearFarAspect = fovNearFarAspect
			end,
			RpcComplete = function ()
				print("RepositoryDaemon: Completing ...")
				local database = MakeDatabase(databasePath, sqlTemplatePath)
				-- write database
				if #names ~= 0 then
					for k, item in ipairs(names) do
						print("AddName: " .. item as { string }[3])
					end
					database:Execute("insert into `Names` values (?, ?, ?)", names as HoneyData)
				end

				if #resources ~= 0 then
					database:Execute("insert into `Resources` values (?, ?, ?)", resources as HoneyData)
				end

				if #entityGroups ~= 0 then
					database:Execute("insert into `EntityGroups` values (?, ?)", entityGroups as HoneyData)
				end
				
				print("RepositoryDaemon: Collecting resource dependencies ...")
				local resourceDependencies : { { number } } = {}
				for path, index in pairs(resourceMap) do
					local res : ResourceBase = SnowyStream.NewResource(path, "", false)	
					if res then
						local info = SnowyStream.InspectResource(res) as InspectResult
						for _, value in ipairs(info.Depends) do
							local target = resourceMap[value]
							if target then
								table.insert(resourceDependencies, { index, target })
							else
								print("RepositoryDaemon: Cannot load resource Dependency: " .. value)
							end
						end
					else
						print("RepositoryDaemon: Cannot load resource: " .. path)
					end
				end

				if #resourceDependencies ~= 0 then
					database:Execute("insert into `ResourceDependencies` values (?, ?)", resourceDependencies as HoneyData)
				end

				print("RepositoryDaemon: Writing components ...")

				if #entityGroups ~= 0 then
					database:Execute("insert into `EntityGroups` values (?, ?)", entityGroups as HoneyData)
				end

				if #components ~= 0 then
					database:Execute("insert into `Components` values (?, ?)", components as HoneyData)
				end

				if #modelComponents ~= 0 then
					for _, v in ipairs(modelComponents) do
						local id = resourceMap[v[2] as string .. ".MeshResource"]
						if id then
							v[2] = id
						else
							print("Unable to find Mesh ID for " .. v[2] as string)
						end
					end

					database:Execute("insert into `ModelComponents` values (?, ?, ?)", modelComponents as HoneyData)
				end

				if #modelComponentMaterials ~= 0 then
					for _, v in ipairs(modelComponentMaterials) do
						local id = resourceMap[v[3] as string .. ".MaterialResource"]
						if id then
							v as { number }[3] = id
						else
							print("Unable to find Material ID for " .. v[3] as string)
						end
					end

					database:Execute("insert into `ModelComponentMaterials` values (?, ?, ?)", modelComponentMaterials as HoneyData)
				end

				if #transformComponents ~= 0 then
					database:Execute("insert into `TransformComponents` values (?, ?, ?, ?, ?, ?, ?, ?)", transformComponents as HoneyData)
				end
				
				if #spaceComponents ~= 0 then
					database:Execute("insert into `SpaceComponents` values (?, ?)", spaceComponents as HoneyData)
				end

				if #envCubeComponents ~= 0 then
					for _, v in ipairs(envCubeComponents) do
						local id = resourceMap[v[2] as string .. ".TextureResource"]
						if id then
							v as { number }[2] = id
						else
							print("Unable to find Texture ID for " .. v[2] as string)
						end
					end

					database:Execute("insert into `EnvCubeComponents` values(?, ?)", envCubeComponents as HoneyData)
				end

				print("RepositoryDaemon: Writing entities ...")

				if #entities ~= 0 then
					database:Execute("insert into `Entities` values (?, ?, ?)", entities as HoneyData)
				end

				if #entityComponents ~= 0 then
					database:Execute("insert into `EntityComponents` values (?, ?)", entityComponents as HoneyData)
				end

				print("RepositoryDaemon: Completed!")
			end
		}

		GalaxyWeaver.SetWeaverRpcCallback(weaver, function (operation : string, ...)
			local handler = handlers[operation] as function
			if handler then
				handler(...)
			end
		end)

		GalaxyWeaver.SetWeaverConnectionCallback(weaver, function (...)
		end)

		GalaxyWeaver.StartWeaver(weaver)
	else
		print("RepositoryDaemon: Unable to create remote sync.")
	end

	return weaver
end

function RepositoryDaemon:Start()

end

return RepositoryDaemon