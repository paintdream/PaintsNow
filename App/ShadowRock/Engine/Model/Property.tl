local Property = record
	name : string
	value : number
	_updater : function (Property)
	_inputs : { Property }
	_outputs : { Property }
end

function Property:UpdaterConst() end

function Property:UpdaterAdd()
	local sum = 0
	for _, v in ipairs(self._inputs) do
		sum = sum + v.value
	end

	self.value = sum
end

function Property:UpdaterMultiply()
	local result = 1
	for _, v in ipairs(self._inputs) do
		result = result * v.value
	end

	self.value = result
end

local min = math.min
function Property:UpdateMin()
	assert(#self._inputs ~= 0)
	local result = self._inputs[1].value
	for _, v in ipairs(self._inputs) do
		result = min(result, v.value)
	end

	self.value = result
end

function Property:UpdateClampMin()
	assert(#self._inputs ~= 0)
	local result = self.value
	for _, v in ipairs(self._inputs) do
		result = min(result, v.value)
	end

	self.value = result
end

local max = math.max
function Property:UpdateMax()
	assert(#self._inputs ~= 0)
	local result = self._inputs[1].value
	for _, v in ipairs(self._inputs) do
		result = max(result, v.value)
	end

	self.value = result
end

function Property:UpdateClampMax()
	assert(#self._inputs ~= 0)
	local result = self.value
	for _, v in ipairs(self._inputs) do
		result = max(result, v.value)
	end

	self.value = result
end

function Property:Update()
	assert(next(self._inputs) == nil)
	local lockMap : { Property : number } = {}
	local updateQueue : { Property } = { self }

	while next(updateQueue) ~= nil do
		local property = table.remove(updateQueue)
		property:_updater()

		for _, sub in ipairs(property._outputs) do
			local lockCount = (lockMap[sub] or #sub._inputs) - 1
			if lockCount == 0 then
				lockMap[sub] = nil as number
				table.insert(updateQueue, sub)
			else
				lockMap[sub] = lockCount
			end
		end
	end

	assert(next(lockMap) == nil) -- all properties must be updated
end

function Property:AddInput(property : Property)
	table.insert(self._inputs, property)
	table.insert(property._outputs, self)
end

function Property:RemoveInput(property : Property)
	for k, v in ipairs(self._inputs) do
		if v == property then
			table.remove(self._inputs, k)
		end
	end

	for k, v in ipairs(property._outputs) do
		if v == self then
			table.remove(self._inputs, k)
		end
	end
end

function Property:AddOutput(property : Property)
	property:AddInput(self)
end

function Property:RemoveOutput(property : Property)
	property:RemoveInput(self)
end

function Property:RemoveAllInputs()
	for k, v in ipairs(self._inputs) do
		for j, w in ipairs(v._outputs) do
			if w == self then
				table.remove(v._outputs, j)
				break
			end
		end
	end

	self._inputs = {}
end

function Property:RemoveAllOutputs()
	for k, v in ipairs(self._outputs) do
		for j, w in ipairs(v._inputs) do
			if w == self then
				table.remove(v._inputs, j)
				break
			end
		end
	end

	self._outputs = {}
end

function Property.New(name : string, value : number, update : function (Property)) : Property
	return setmetatable({
		name = name,
		value = value,
		_updater = update,
		_inputs = {},
		_outputs = {},
	}, Property as METATABLE) as Property
end

return Property