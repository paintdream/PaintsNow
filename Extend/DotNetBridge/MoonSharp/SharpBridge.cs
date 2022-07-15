using System;
using DotNetBridge;

namespace MoonSharp
{
	public class SharpBridge
	{
		public string GetVersion(int a, float b)
		{
			return string.Format("SharpBridge {0} - {1}", a, b);
		}

		public void TestVersion()
		{
			var leavesBridge = new LeavesBridge();
			Console.WriteLine("Hello World! " + leavesBridge.GetScriptHandle());
		}
	}
}
