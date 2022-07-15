local function Expand(t, newLine, depth)
	if type(t) == "table" and depth ~= 0 then
		local convert = {}
		for i, v in pairs(t) do
			if type(v) == "nil" then
				v = "nil"
			end

			m = Expand(v, false, depth - 1)
			table.insert(convert, newLine and m or ("[" .. i .. "] = " .. m))
		end

		if newLine then
			return table.concat(convert, '\n')
		else
			return "{ " .. table.concat(convert, ', ') .. " }"
		end
	else
		return tostring(t)
	end
end

if System and System.Print then
	print = function (...)
		local args = { ... }
		if #args ~= 0 then
			return System.Print(Expand(args, true, 4))
		end
	end
end
