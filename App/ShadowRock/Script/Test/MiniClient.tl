local Test = record end
local TcpWalker = require("Script/Network/TcpWalker")

function Test.Main()
	local host = "http://localhost/shadowrock/"
	local login = host .. "wp-login.php"
	local event = host .. "wp-admin/admin-ajax.php"

	local dispatcher = EchoLegend.OpenDispatcher(false)
	dispatcher:ActivateDispatcher()
	local tcpWalker = TcpWalker.New(dispatcher, login, event, "root", "root")
	print("Start login ...")

	if tcpWalker:Login() then
		print("Login SUCCESS!")
	else
		print("Login FAILED!")
	end
end

return Test