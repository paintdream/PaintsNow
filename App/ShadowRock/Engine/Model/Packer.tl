-- Packer.tl
-- Import raw data, save as reflected resource data
--

local Packer = {}

local function NeedUpdate(srcPath : string, dstPath : string) : boolean
	local src : File = SnowyStream.NewFile(srcPath, false)
	local dst : File = SnowyStream.NewFile(dstPath, false)
	local result = true
	if src and dst then
		local srcLastModifiedTime : number = SnowyStream.GetFileLastModifiedTime(src)
		local dstLastModifiedTime : number = SnowyStream.GetFileLastModifiedTime(dst)
		result = srcLastModifiedTime > dstLastModifiedTime
	end

	if src then
		SnowyStream.CloseFile(src)
	end

	if dst then
		SnowyStream.CloseFile(dst)
	end

	return result
end

local function NewResourceHandler(type : string) : function (string, string, string) : (boolean)
	return function (src : string, dst : string, ext : string) : boolean
		local dstWithExt = dst .. "." .. type .. ".pod"
		if not NeedUpdate(src, dstWithExt) then
			return true
		end
		
		print("Packing [" .. ext:lower() .. "] => " .. dst)

		local res : ResourceBase = SnowyStream.NewResource(dst, type, true)
		local success = false
		if res then
			local data = SnowyStream.FetchFileData(src) as string
			if data then
				if SnowyStream.LoadExternalResourceData(res, data) then
					if SnowyStream.PersistResource(res, "") then
						success = true
					end
				else
					print("Couldn't merge external resource data: " .. src)
				end
			else
				print("Couldn't load external resource data: " .. src)
			end
		else
			print("Couldn't create resource: " .. src)
		end

		return success
	end
end

local HandlerTextureResource = NewResourceHandler("TextureResource")
local HandlerFontResource = NewResourceHandler("FontResource")

local dispatchers : { string : function (string, string, string) : boolean } = {
	["ttf"] = HandlerFontResource,
	["otf"] = HandlerFontResource,
	["ttc"] = HandlerFontResource,
	["jpg"] = HandlerTextureResource,
	["png"] = HandlerTextureResource,
	["tiff"] = HandlerTextureResource
}

local function DoImport(src : string, dest : string)
	-- file
	if src:sub(#src) == "/" then
		for index, file in ipairs(SnowyStream.QueryFiles(src) as { string }) do
			if index and file then
				DoImport(src .. file, dest .. file)
			end
		end
	else
		-- parse extension
		local folder, name, ext = src:match('(.-/)(.*)%.(.*)')
		if folder and name and ext then
			local dispatcher = dispatchers[ext]
			if dispatcher then
				local dfolder, dname : string, string = dest:match('(.-/)(.*)%.(.*)')
				if dfolder and dname then
					dispatcher(src, dfolder .. dname, ext:lower())
				end
			end
		end
	end
end

function Packer:Import(src : string, dest : string)
	DoImport(src, dest)
end

function Packer:Pack(src : string, format : string)
	-- pack scripts and assets
	
end

return Packer