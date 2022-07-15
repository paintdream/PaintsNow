local Database = record
	Handle : Hive
	__index : any
end

Database.__index = Database

function Database:Execute(sql : string, data : HoneyData) : Honey
	return self.Handle:Execute(sql, data)
end

function Database:Lines(sql : string, cacheCount : number, data : HoneyData) : function (any, number) : (any, any)
	cacheCount = cacheCount or 0

	local honey = self:Execute(sql, data as HoneyData)
	if not honey then return function (t : any, i : number) : (any, any) return nil, nil end end

	local cache : any

	return function (t, k : number) : (any, any)
		if k == nil as number or k == cacheCount then
			-- reinit
			local s : any = honey:Step(cacheCount)
			cache = s
			k = nil as number
		end

		-- tail call to next
		return next(cache as { any : any}, k)
	end
end

function Database:Commit()

end

function Database:Query(sql : string, data : HoneyData)
	print("Query Result: ")
	print("--------------------------------------")
	local rowCount = 0
	for k, line in self:Lines(sql, 5, data) do
		local item = ""
		for i, v in ipairs(line as { string }) do
			if i == 1 then
				item = tostring(v)
			else
				item = item .. "\t|" .. tostring(v)
			end
		end

		print(item)
		rowCount = rowCount + 1
	end
	print("Lines: " .. tostring(rowCount))
	print("--------------------------------------")
end

function Database.New(path : string) : Database
	local handle : Hive = Remembery.NewDatabase(path, "", "", true)
	if not handle then return nil as Database end

	local meta = {
		Handle = handle
	}
	return setmetatable(meta, Database as METATABLE) as Database
end

return Database