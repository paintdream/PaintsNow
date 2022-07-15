#include "ServiceWin32.h"
#include "../../../../Utility/LeavesFlute/Loader.h"

#ifdef _WIN32
#include <TlHelp32.h>
using namespace PaintsNow;

ServiceWin32::ServiceWin32(const String& name, const String& description) : serviceName(name), serviceDescription(description), runningEvent(nullptr) {}

ServiceWin32& ServiceWin32::GetInstance() {
	static ServiceWin32 theServiceWin32("LeavesWind", "LeavesWind: A Scriptable Distributed Computing System based on PaintsNow");
	return theServiceWin32;
}

static void WINAPI ServiceCtrlHandler(DWORD opcode) {
	ServiceWin32::GetInstance().ServiceCtrlHandler(opcode);
}

void WINAPI ServiceMain(DWORD argc, LPSTR* argv) {
	ServiceWin32::GetInstance().ServiceMain(argc, argv);
}

bool ServiceWin32::RunServiceWorker(DWORD argc, LPSTR* argv) {
	CmdLine cmdLine;
	cmdLine.Process((int)argc, argv);

	Loader loader;
	loader.consoleHandler = Wrap(this, &ServiceWin32::ConsoleHandler);
	loader.setupHandler = Wrap(this, &ServiceWin32::SetupHandler);
	loader.Run(cmdLine);

	return true;
}

void ServiceWin32::RunScriptRegisterExternal(Loader& loader) {
	loader.setupHandler = Wrap(this, &ServiceWin32::SetupHandler);
}

bool ServiceWin32::RunServiceMaster(DWORD argc, LPSTR* argv) {
	SERVICE_TABLE_ENTRYA dispatchTable[] = {
		{ (LPSTR)serviceName.c_str(), &::ServiceMain },
		{ NULL, NULL }
	};

	return ::StartServiceCtrlDispatcherA(dispatchTable) != 0;
}

void ServiceWin32::SetupHandler(LeavesFlute& leavesFlute) {
	IScript::Request& request = leavesFlute.GetInterfaces().script.GetDefaultRequest();
	request.DoLock();
	request << global;
	request << key("System") >> begintable << key("ToolkitWin32");
	toolkitWin32.ScriptRequire(request); // register callbacks
	request << endtable << endtable;
	request.UnLock();
}

void ServiceWin32::ConsoleHandler(LeavesFlute& leavesFlute) {
	MSG msg;
	if (runningEvent == nullptr) {
		while (::GetMessageW(&msg, NULL, 0, 0)) {
			toolkitWin32.HandleMessage(leavesFlute, msg.message, msg.wParam, msg.lParam);
		}
	} else {
		while (true) {
			DWORD event = ::MsgWaitForMultipleObjects(1, &runningEvent, FALSE, INFINITE, QS_ALLINPUT);
			if (event == WAIT_OBJECT_0) break;

			::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
			toolkitWin32.HandleMessage(leavesFlute, msg.message, msg.wParam, msg.lParam);
		}
	}
}

void ServiceWin32::ServiceMain(DWORD argc, LPSTR* argv) {
	WCHAR currentPath[MAX_PATH * 2];
	::GetModuleFileNameW(nullptr, currentPath, MAX_PATH * 2);
	WCHAR* t = wcsrchr(currentPath, L'\\');
	if (t != nullptr) *t = L'\0';
	::SetCurrentDirectoryW(currentPath);
	// freopen("service.log", "w", stdout);

	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	String name = Utf8ToSystem(serviceName);
	serviceStatusHandle = ::RegisterServiceCtrlHandlerW((WCHAR*)name.c_str(), ::ServiceCtrlHandler);
	if (serviceStatusHandle != (SERVICE_STATUS_HANDLE)0) {
		serviceStatus.dwCurrentState = SERVICE_RUNNING;
		serviceStatus.dwCheckPoint = 0;
		serviceStatus.dwWaitHint = 0;
		::SetServiceStatus(serviceStatusHandle, &serviceStatus);
	}

	runningEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
	RunServiceWorker(argc, argv);
	::CloseHandle(runningEvent);
}

void ServiceWin32::ServiceCtrlHandler(DWORD opcode) {
	switch (opcode) {
		case SERVICE_CONTROL_PAUSE:
			// printf("Service Control Pause\n");
			serviceStatus.dwCurrentState = SERVICE_PAUSED;
			::SetServiceStatus(serviceStatusHandle, &serviceStatus);
			break;
		case SERVICE_CONTROL_CONTINUE:
			// printf("Service Control Continue\n");
			serviceStatus.dwCurrentState = SERVICE_RUNNING;
			::SetServiceStatus(serviceStatusHandle, &serviceStatus);
			break;
		case SERVICE_CONTROL_STOP:
			// printf("Service Control Stop\n");
			serviceStatus.dwWin32ExitCode = 0;
			serviceStatus.dwCurrentState = SERVICE_STOPPED;
			serviceStatus.dwCheckPoint = 0;
			serviceStatus.dwWaitHint = 0;
			::SetServiceStatus(serviceStatusHandle, &serviceStatus);
			::SetEvent(runningEvent);
			break;
		case SERVICE_CONTROL_INTERROGATE:
			break;
	}
}

bool ServiceWin32::InstallService() {
	WCHAR fullPath[MAX_PATH * 2];
	::GetModuleFileNameW(nullptr, fullPath, MAX_PATH * 2);
	SC_HANDLE scManager = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (scManager == nullptr) return false;

	String name = Utf8ToSystem(serviceName);
	SC_HANDLE scService = ::CreateServiceW(scManager, (WCHAR*)name.c_str(), (WCHAR*)name.c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, fullPath, nullptr, nullptr, nullptr, nullptr, nullptr);

	if (scService == nullptr) {
		::CloseServiceHandle(scManager);
		return false;
	}

	::CloseServiceHandle(scService);
	::CloseServiceHandle(scManager);
	return true;
}

bool ServiceWin32::DeleteService() {
	SC_HANDLE scManager = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (scManager == nullptr) return false;

	String name = Utf8ToSystem(serviceName);
	SC_HANDLE scService = ::OpenServiceW(scManager, (WCHAR*)name.c_str(), SERVICE_ALL_ACCESS);

	if (scService == nullptr) {
		::CloseServiceHandle(scManager);
		return false;
	}

	bool result = ::DeleteService(scService) != 0;

	::CloseServiceHandle(scService);
	::CloseServiceHandle(scManager);
	return result;
}

bool ServiceWin32::InServiceContext() {
	DWORD processID = ::GetCurrentProcessId();
	DWORD parentID = 0;
	HANDLE h = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe = { 0 };
	std::vector<DWORD> serviceIDs;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (::Process32First(h, &pe)) {
		do {
			if (pe.th32ProcessID == processID) {
				parentID = pe.th32ParentProcessID;
			} else if (_stricmp(pe.szExeFile, "SERVICES.EXE") == 0) {
				serviceIDs.emplace_back(pe.th32ProcessID);
			}
		} while (::Process32Next(h, &pe));
	}
	::CloseHandle(h);

	return std::find(serviceIDs.begin(), serviceIDs.end(), parentID) != serviceIDs.end();
}

#endif