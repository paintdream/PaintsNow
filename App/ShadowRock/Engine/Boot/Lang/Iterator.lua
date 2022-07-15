function SortedPairs(input, comp)
	local keys = {}
	for k in pairs(input) do
		table.insert(keys, k)
	end

	table.sort(keys, comp)
	local i = 0

	return function ()
		i = i + 1
		local v = keys[i]
		return v, input[v]
	end
end