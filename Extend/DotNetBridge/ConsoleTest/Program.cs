using System;
using DotNetBridge;

namespace ConsoleTest
{
	class Program
	{
		static void Main(string[] args)
		{
			var leavesBridge = new LeavesBridge();
			Console.WriteLine("Hello World! " + leavesBridge.GetScriptHandle());
		}
	}
}
