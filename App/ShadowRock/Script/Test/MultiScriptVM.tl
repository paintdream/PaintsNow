local Test = record end

function Test.Main()
	local component = RemoteComponentModule.New(false, 0)
	local func = component:Load("print(...) return 2345")
	component:Call(func, "1234")
	local running = coroutine.running()
	component:CallAsync(function (value)
		print("RET: " .. tostring(value))
		coroutine.resume(running)
	end, func, "0123")

	coroutine.yield()
	print("NEXT")

	local backward = component:Load([[
		local function test(callback)
		   SysCall(callback, "It' me!")
		end
		test(...)
	]])

	component:Call(backward, function (msg : string)
	   print("GET MSG: " .. msg)
	end)

	local transparent = RemoteComponentModule.New(true, 0)
	local foo = transparent:Load([[
		local function test(callback)
		   callback("It' me~~~!")
		end
		test(...)
	]])
	transparent:Call(foo, function (msg : string)
	   print("GET MSG: " .. msg)
	end)
	
	local CrossRoutine = require("Engine/Controller/CrossRoutine")

	-- CrossRoutine is running on another LuaVM
	local upval = "~~~"
	local cross = CrossRoutine.New(nil as function(any))
	local rpc = cross(function (value : string) : string
		return "Hello " .. value .. upval
	end)

	print("MSG " .. rpc("World!") as string)
	print("MSG " .. rpc("NewWorld!") as string)

	print("Testing CrossDispatcher~")
	local CrossDispatcher = require("Engine/Controller/CrossDispatcher")
	local dispatcher = CrossDispatcher.New(4)
	print("Register dispatcher function ...")
	dispatcher:Register("test", function (i)
		print("Task " .. tostring(i) .. " finished.")
	end)

	print("Dispatching ...")
	for i = 1, 1000 do
		dispatcher:Dispatch("test", i)
	end

	print("Waiting ...")
	dispatcher:Wait()

	print("Finished.")
end

return Test