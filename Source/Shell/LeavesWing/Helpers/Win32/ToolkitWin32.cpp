#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _WIN32
#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#define SwitchToThread() false
#endif

#include "ToolkitWin32.h"

#ifdef _WIN32
using namespace PaintsNow;

ToolkitWin32::ToolkitWin32() {
	mainThreadID = ::GetCurrentThreadId();
}

ToolkitWin32::~ToolkitWin32() {}

uint32_t ToolkitWin32::GetMainThreadID() const {
	return mainThreadID;
}

void ToolkitWin32::ScriptInitialize(IScript::Request& request) {}

void ToolkitWin32::ScriptUninitialize(IScript::Request& request) {
	if (messageListener) {
		request.Dereference(messageListener);
	}
}

TObject<IReflect>& ToolkitWin32::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestGetMainThreadID)[ScriptMethod = "GetMainThreadID"];
		ReflectMethod(RequestGetHostPath)[ScriptMethod = "GetHostPath"];
		ReflectMethod(RequestGetSystemInfo)[ScriptMethod = "GetSystemInfo"];
		ReflectMethod(RequestExit)[ScriptMethod = "Exit"];
		ReflectMethod(RequestListenMessage)[ScriptMethod = "ListenMessage"];
		ReflectMethod(RequestPostThreadMessage)[ScriptMethod = "PostThreadMessage"];
		ReflectMethod(RequestCreateProcess)[ScriptMethod = "CreateProcess"];
		ReflectMethod(RequestCloseHandle)[ScriptMethod = "CloseHandle"];
		ReflectMethod(RequestWaitForSingleObject)[ScriptMethod = "WaitForSingleObject"];
		ReflectMethod(RequestTerminateProcess)[ScriptMethod = "TerminateProcess"];
	}

	return *this;
}

uint32_t ToolkitWin32::RequestGetMainThreadID(IScript::Request& request) {
	return GetMainThreadID();
}

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

static String GetProcessArchitecture(WORD arch) {
	switch (arch) {
		case PROCESSOR_ARCHITECTURE_AMD64:
			return "x64";
		case PROCESSOR_ARCHITECTURE_ARM:
			return "ARM";
		case PROCESSOR_ARCHITECTURE_ARM64:
			return "ARM64";
		case PROCESSOR_ARCHITECTURE_INTEL:
			return "x86";
		default:
			return "Unknown";
	}
}

static std::pair<String, String> GetProcessorAndOSName() {
	// https://docs.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer
	std::pair<String, String> name;
	HRESULT hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

	if (FAILED(hres)) {
		return name;
	}

	IWbemLocator* pLoc = NULL;

	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres)) {
		return name;
	}

	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

	if (FAILED(hres)) {
		pLoc->Release();
		return name;
	}

	hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		return name;
	}

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;

	IEnumWbemClassObject* pEnumerator = NULL;

	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Processor"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		return name;
	}

	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn) {
			break;
		}

		VARIANT vtProp;

		// Get the value of the Name property
		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		name.first = SystemToUtf8(String((const char*)vtProp.bstrVal, SysStringLen(vtProp.bstrVal) * 2));
		VariantClear(&vtProp);

		pclsObj->Release();
	}

	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_OperatingSystem"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		return name;
	}

	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn) {
			break;
		}

		VARIANT vtProp;

		// Get the value of the Name property
		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		name.second = SystemToUtf8(String((const char*)vtProp.bstrVal, SysStringLen(vtProp.bstrVal) * 2));
		VariantClear(&vtProp);

		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();

	return name;
}

void ToolkitWin32::RequestGetSystemInfo(IScript::Request& request) {
	SYSTEM_INFO systemInfo;
	::GetSystemInfo(&systemInfo);

	String computerName = "PC";
	WCHAR name[MAX_PATH + 1] = { 0 };
	DWORD length = MAX_PATH;
	if (::GetComputerNameW(name, &length)) {
		computerName = SystemToUtf8(String((const char*)name, length * 2));
	}

	MEMORYSTATUSEX memory;
	memory.dwLength = sizeof(memory);
	::GlobalMemoryStatusEx(&memory);

	std::pair<String, String> processorAndOSName = GetProcessorAndOSName();

	request.DoLock();
	request << begintable;
	request << key("ComputerName") << computerName
		<< key("NumberOfProcessors") << (int32_t)systemInfo.dwNumberOfProcessors
		<< key("ProcessorArchitecture") << GetProcessArchitecture(systemInfo.wProcessorArchitecture)
		<< key("PageSize") << (int32_t)systemInfo.dwPageSize
		<< key("TotalMemory") << memory.ullTotalPhys
		<< key("AvaliableMemory") << memory.ullAvailPhys
		<< key("ProcessorName") << processorAndOSName.first
		<< key("OperatingSystem") << processorAndOSName.second;
	request << endtable;
	request.UnLock();
}

void ToolkitWin32::HandleMessage(LeavesFlute& leavesFlute, uint32_t msg, uint64_t wParam, uint64_t lParam) {
	if (messageListener) {
		IScript::Request& request = *leavesFlute.bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(messageListener, msg, wParam, lParam);
		request.Pop();
		request.UnLock();
		leavesFlute.bridgeSunset.requestPool.ReleaseSafe(&request);
	}
}

void ToolkitWin32::RequestListenMessage(IScript::Request& request, IScript::Request::Ref callback) {
	if (messageListener) {
		request.DoLock();
		request.Dereference(messageListener);
		request.UnLock();
	}

	messageListener = callback;
}

void ToolkitWin32::RequestExit(IScript::Request& request) {
	::PostThreadMessageW(mainThreadID, WM_QUIT, 0, 0);
}

void ToolkitWin32::RequestPostThreadMessage(IScript::Request& request, uint64_t thread, uint32_t msg, uint64_t wParam, uint64_t lParam) {
	::PostThreadMessageW((DWORD)thread, (UINT)msg, (WPARAM)wParam, (LPARAM)lParam);
}

String ToolkitWin32::RequestGetHostPath(IScript::Request& request) {
	WCHAR hostPath[MAX_PATH * 2];
	::GetModuleFileNameW(nullptr, hostPath, MAX_PATH * 2 - 2);
	return SystemToUtf8(String((const char*)hostPath, wcslen(hostPath) * 2));
}

std::pair<uint64_t, uint64_t> ToolkitWin32::RequestCreateProcess(IScript::Request& request, const String& path, const String& parameter, const String& currentPath) {
	String cmdLine = Utf8ToSystem(path);
	String directory = Utf8ToSystem(currentPath);
	STARTUPINFOW info = { 0 };
	info.cb = sizeof(STARTUPINFOW);
	info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	info.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
	info.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
	info.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
	info.wShowWindow = parameter == "Hide" ? SW_HIDE : SW_SHOW;
	PROCESS_INFORMATION pi = { 0 };

	WCHAR buffer[MAX_PATH * 2] = { 0 };
	if (path.empty()) {
		::GetModuleFileNameW(nullptr, buffer, MAX_PATH * 2);
	} else {
		wcsncpy(buffer, (WCHAR*)cmdLine.c_str(), MAX_PATH * 2);
	}

	if (::CreateProcessW(nullptr, buffer, nullptr, nullptr, TRUE, 0, nullptr, currentPath.empty() ? nullptr : (WCHAR*)directory.c_str(), &info, &pi)) {
		::CloseHandle(pi.hThread);
		return std::make_pair((uint64_t)pi.hProcess, (uint64_t)pi.dwThreadId);
	} else {
		return std::make_pair(0, 0);
	}
}

void ToolkitWin32::RequestCloseHandle(IScript::Request& request, uint64_t object) {
	::CloseHandle((HANDLE)object);
}

uint32_t ToolkitWin32::RequestWaitForSingleObject(IScript::Request& request, uint64_t object, uint64_t timeout) {
	return ::WaitForSingleObject((HANDLE)object, timeout > 0xffffffff ? INFINITE : (DWORD)timeout);
}

bool ToolkitWin32::RequestTerminateProcess(IScript::Request& request, uint64_t process, uint32_t exitCode) {
	return ::TerminateProcess((HANDLE)process, exitCode) != 0;
}

#endif
