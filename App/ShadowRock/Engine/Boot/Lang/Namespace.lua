-- Import All Modules and functions in System to global table for typedlua routing ...
local function ImportSymbols(env, key, collection)
	if collection.__delegate__ then
		-- print("Symbol imported for module " .. key)
		for k, v in pairs(collection) do
			env[k] = v
			if type(v) == "table" then
				ImportSymbols(env, k, v)
			end
		end
	end
end

ImportSymbols(_ENV, "System", System)

function using(module, ...)
	if type(module) == "string" then
		return using(require(module, ...))
	else
		assert(type(module) == "table")
		for k, v in pairs(module) do
			if rawget(_ENV, k) ~= nil then
				error("Module.using(): Global variable " .. k .. " collision detected.", 1)
			end

			_ENV[k] = v
		end

		return module
	end
end
