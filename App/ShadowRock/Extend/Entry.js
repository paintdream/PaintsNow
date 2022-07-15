function Main() {
	window.System.Print("Hello, JavaScript!");
	window.System.SetAppTitle("JScript Demo");
	// open console
	window.System.ListenConsole(function (self, text) {
		window.System.Print("Console Input: " + text)
	});
}