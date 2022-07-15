local Property = require("Engine/Model/Property")

local PropertyGraph = record
	_root : Property
	_committed : boolean
	_properties : { string : Property }
end

function PropertyGraph:Commit()
	self._root:RemoveAllOutputs()

	for k, v in pairs(self._properties) do
		local length = #v._inputs
		if length == 0 then
			self._root:AddOutput(v)
		end
	end

	self._committed = true
end

function PropertyGraph:Update()
	assert(self._committed)
	self._root:Update()
end

function PropertyGraph:GetProperty(name : string) : Property
	return self._properties[name]
end

function PropertyGraph:AddProperty(property : Property)
	self._properties[property.name] = property
	self._committed = false
end

function PropertyGraph:RemoveProperty(property : Property)
	self._properties[property.name] = nil as Property
	self._committed = false
end

function PropertyGraph.New() : PropertyGraph
	-- create root property
	return setmetatable({
		_root = Property.New("__root__", 0, Property.UpdaterConst),
		_committed = false,
		_properties = {}
	}, PropertyGraph as METATABLE) as PropertyGraph
end

return PropertyGraph