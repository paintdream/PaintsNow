local ResourceCache = record
	type : string
	resources : { string : ResourceBase }	
end

function ResourceCache:Clear()
	self.resources = {}
end

function ResourceCache:Load(path : string) : ResourceBase
	local exist = self.resources[path]
	if exist then return exist end

	exist = SnowyStream.NewResource(path, self.type, false) as ResourceBase
	self.resources[path] = exist
	return exist
end

function ResourceCache.New(resourceType : string) : ResourceCache
	return setmetatable({
		type = resourceType,
		resources = {}
	}, ResourceCache as METATABLE) as ResourceCache
end

return ResourceCache