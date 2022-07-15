local Json = record end

function Json.Parse(str : string) : { any : any }
	return SnowyStream.ParseJson(str) as { any : any }
end

local function Stringify(data : { any : any }) : string
	local str = "{"
	for k, v in pairs(data) do
		if type(k) == "number" or type(k) == "string" then
			local item =  "\"" .. tostring(k) .. "\" : " 
			if type(v) == "number" or type(v) == "string" then
				item = item .. "\"" .. tostring(v) .. "\""
			elseif type(v) == "table" then
				item = item .. Stringify(v as { any : any })	
			end	

			str = str .. item
		end
	end

	str = str .. "}"

	return str
end

function Json:Stringify(data : { any : any }) : string
	return Stringify(data)
end

return Json