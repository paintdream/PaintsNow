-- save extra space introduced by typedlua
-- deprecated
--[[
local orgSetmetatable = setmetatable
setmetatable = function (obj, meta)
	if type(obj) == "userdata" then
		print("Unable to set metatable for userdata. stack:")
		print(debug.traceback())
	end

	local old = getmetatable(obj)
	if old and rawget(meta, "__chain") then
		orgSetmetatable(old, meta)
		return obj
	else
		-- fold continous indexing
		local p = meta.__index
		if type(p) == "table" and p == p.__index then
			meta = p
		end

		return orgSetmetatable(obj, meta)
	end
end
]]