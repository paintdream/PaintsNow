local Test = record end
local DotNetBridge = record
	CreateInstance : function (string, string) : any
end

local DotNetInstance = record
	GetVersion : function (number, number) : string
end

function Test.Main()
	local sharp = System.LoadLibrary("../../Extend/DotNetBridge/Debug/DotNetBridge.dll")
	if sharp ~= 0 then
		local main = System.GetSymbolAddress(sharp, "Main")
		local bridge = System.CallSymbol(main, "request", "Initialize") as DotNetBridge
		local instance = bridge.CreateInstance("../../Extend/DotNetBridge/MoonSharp/bin/Debug/net5.0/MoonSharp.dll", "MoonSharp.SharpBridge") as DotNetInstance
		if instance then
			for k, v in pairs(instance) do
				print("-> " .. tostring(k) .. " = " .. tostring(v))
			end
			print("VERSION: " .. instance.GetVersion(1, 2.0))
		end
	end
end

return Test