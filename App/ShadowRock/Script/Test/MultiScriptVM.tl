local Test = record end
local CrossDispatcher = require("Engine/Controller/CrossDispatcher")

function Test.Main()
	local component = CrossScriptModule.New(false, false)
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

	local transparent = CrossScriptModule.New(true, false)
	local foo = transparent:Load([[
		local function test(callback)
		   callback("It' me~~~!")
		end
		test(...)
	]])
	transparent:Call(foo, function (msg : string)
	   print("GET MSG: " .. msg)
	end)

	print("Testing CrossDispatcher~")
	local dispatcher = CrossDispatcher.New(4)
	local base = 2
	print("Register dispatcher function ...")
	local getThread = BridgeSunset.GetCurrentThreadIndex
	local test = dispatcher:Setup(function (i : number) : number
		print("Task " .. tostring(i) .. " finished. on VM " .. tostring(getThread()))
		return i + base
	end)

	print("Dispatching ...")
	local awaitables : { Awaitable } = {}
	for i = 1, 100 do
		table.insert(awaitables, dispatcher:Dispatch(test, i))
	end

	print("Waiting ...")
	for i = 1, 100 do
		print("Wait result: " .. tostring(dispatcher:Wait(awaitables[i])))
	end

	print("Finished.")
end

return Test