string.startswith = function (s, p)
	return string.sub(s, 1, string.len(p)) == p
end

string.endswith = function (s, p)
	return string.sub(s, string.len(s) - string.len(p) + 1) == p
end

string.split = function (str, delimiter)
	if str == nil or str == '' or delimiter == nil then
		return nil
	end
	
	local result = {}
	for match in (str .. delimiter):gmatch("(.-)" .. delimiter) do
		table.insert(result, match)
	end

	return result
end