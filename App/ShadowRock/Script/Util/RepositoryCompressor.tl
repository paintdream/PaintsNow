local RepositoryCompressor = record
	__index : any
end

RepositoryCompressor.__index = RepositoryCompressor

local Database = require("Engine/Model/Database")

function RepositoryCompressor.Compress(type : string, database : Database)
	if database then
		print("\tCompressing resources ...")

		local texID = "1"
		for _, line in database:Lines("select ID from `Names` where `Name` == 'TextureResource'", 1, nil as HoneyData) do
			texID = tostring(line as { number }[1] as number)
		end

		local runningCount = 0
		print("TextureResource Unique ID " .. texID)

		for _, line in database:Lines("select Path from `Resources` where `TypeID` == " .. texID .. ";", 64, nil as HoneyData) do
			local path = line as { number }[1] as string
			local textureResource = SnowyStream.NewResource(path, "TextureResource", false) as ResourceBase
			if textureResource then
				local resource = textureResource
				runningCount = runningCount + 1
				SnowyStream.CompressResourceAsync(textureResource, type or "BPTC", false, function ()
					print("Compressed texture generated: " .. path)
					SnowyStream.PersistResource(resource, "")

					runningCount = runningCount - 1
					if runningCount == 0 then
						print("Compress finished.")
					end
				end)
			end
		end
	end
end


return RepositoryCompressor