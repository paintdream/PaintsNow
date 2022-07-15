// ServiceWin32.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-28
//

#pragma once

#ifdef _WIN32

#include "../../../../Utility/LeavesFlute/LeavesFlute.h"
#include "ToolkitWin32.h"
#include <Windows.h>

namespace PaintsNow {
	class Loader;
	class ServiceWin32 {
	public:
		ServiceWin32(const String& serviceName, const String& serviceDescription);
		void RunScriptRegisterExternal(Loader& loader);
		bool RunServiceMaster(DWORD argc, LPSTR* argv);
		bool RunServiceWorker(DWORD argc, LPSTR* argv);
		void ServiceCtrlHandler(DWORD opcode);
		bool InstallService();
		bool DeleteService();
		static ServiceWin32& GetInstance();
		static bool InServiceContext();

	public:
		void ServiceMain(DWORD argc, LPTSTR* argv);

	protected:
		void ConsoleHandler(LeavesFlute& leavesFlute);
		void SetupHandler(LeavesFlute& leavesFlute);

	protected:
		String serviceName;
		String serviceDescription;
		SERVICE_STATUS serviceStatus;
		SERVICE_STATUS_HANDLE serviceStatusHandle;
		ToolkitWin32 toolkitWin32;
		HANDLE runningEvent;
	};
}

#endif