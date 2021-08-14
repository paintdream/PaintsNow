#include "LeavesFlute.h"
#include <ctime>
#include "../SnowyStream/Manager/RenderResourceManager.h"
#include "../../Core/Driver/Filter/Pod/ZFilterPod.h"
#include "../../General/Driver/Filter/Json/ZFilterJson.h"
#include "../../General/Driver/Filter/LZMA/ZFilterLZMA.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#include <dlfcn.h>
#endif

using namespace PaintsNow;

class ScanModules : public IReflect {
public:
	ScanModules() : IReflect(true, false) {}
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (!s.IsBasicObject() && s.QueryInterface(UniqueType<IScript::Library>()) != nullptr) {
			modules.emplace_back(static_cast<IScript::Library*>(&s));
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	std::vector<IScript::Library*> modules;
};

LeavesFlute::LeavesFlute(bool ng, Interfaces& inters, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const String& defMount, uint32_t threadCount, uint32_t warpCount) :
					ISyncObject(inters.thread),
					interfaces(inters),
					purpleTrail(inters.thread),
					bridgeSunset(inters.thread, inters.script, threadCount, warpCount),
					echoLegend(inters.thread, inters.network, bridgeSunset),
					snowyStream(inters, bridgeSunset, subArchiveCreator, defMount, Wrap(this, &LeavesFlute::OnConsoleOutput)),
					mythForest(inters, snowyStream, bridgeSunset),
					heartVioliner(inters.thread, inters.timer, bridgeSunset),
					remembery(inters.thread, inters.archive, inters.database, bridgeSunset),
					galaxyWeaver(inters.thread, inters.tunnel, bridgeSunset, snowyStream, mythForest),
					consoleThread(nullptr), rawPrint(false)
{
	ScanModules scanModules;
	(*this)(scanModules);
	std::swap(modules, scanModules.modules);
	Initialize();

	interfaces.frame.SetCallback(this);
	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	Require(request); // register callbacks
	request.UnLock();
}

void LeavesFlute::EnableRawPrint(bool enable) {
	rawPrint = enable;
}

/*
void LeavesFlute::Reset(bool reload) {
	resetting = true;
	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	if (listenConsole) {
		request.Dereference(listenConsole);
	}
	ScriptUninitialize(request);
	request.UnLock();
	interfaces.script.Reset();

	if (reload) {
		IScript::Request& request = interfaces.script.GetDefaultRequest();

		request.DoLock();
		ScriptInitialize(request);
		request.UnLock();
	}

	resetting = false;
}
*/

LeavesFlute::~LeavesFlute() {
	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	if (listenConsole) {
		request.Dereference(listenConsole);
	}
	request.UnLock();
	interfaces.script.Reset();

	Uninitialize();
}

void LeavesFlute::Setup() {
	if (setupHandler) {
		setupHandler(*this);
	}
}

void LeavesFlute::EnterStdinLoop() {
	if (consoleHandler) {
		consoleHandler(*this);
	} else {
		printf("Init Standard Input Environment ...\n");
		ConsoleProc(nullptr, 0);
	}
}

void LeavesFlute::EnterMainLoop() {
	assert(!consoleHandler);
	BeginConsole();
	interfaces.frame.EnterMainLoop();

#ifdef _WIN32
	::FreeConsole();
	// interfaces.thread.Wait(consoleThread);
#endif
	EndConsole();
}

void LeavesFlute::BeginConsole() {
	assert(consoleThread == nullptr);
	consoleThread = interfaces.thread.NewThread(Wrap(this, &LeavesFlute::ConsoleProc), 0);
}

void LeavesFlute::EndConsole() {
	assert(consoleThread != nullptr);
	threadApi.DeleteThread(consoleThread);
	consoleThread = nullptr;
}

#include <iostream>
#ifdef WIN32
#include <Windows.h>

bool ReadConsoleSafe(WCHAR buf[], DWORD size, DWORD* read, HANDLE h) {
	// may crash or memory leak when user close the window on win7
	// but why ?
	__try {
		if (::ReadConsoleW(h, buf, size - 1, read, nullptr)) {
			return true;
		}
	} __except (EXCEPTION_ACCESS_VIOLATION) {
		printf("Unexpected access violation\n");
	}

	return false;
}

#else
#include <sys/poll.h>
#endif

bool LeavesFlute::ProcessCommand(const String& command) {
	if (consoleThread == nullptr && (command == "quit" || command == "exit")) {
		return false;
	} else {
		IScript::Request& request = interfaces.script.GetDefaultRequest();
		if (!listenConsole) {
			request.DoLock();
			IScript::Request::Ref code = request.Load(command, "Console");
			request.UnLock();

			if (code) {
				bridgeSunset.threadPool.Dispatch(CreateTaskScriptOnce(code));
			}
		} else {
			bridgeSunset.threadPool.Dispatch(CreateTaskScript(listenConsole, command));
		}

		return true;
	}
}

void LeavesFlute::OverrideSetupProc(const TWrapper<void, LeavesFlute&>& handler) {
	setupHandler = handler;
}

void LeavesFlute::OverrideConsoleProc(const TWrapper<void, LeavesFlute&>& handler) {
	consoleHandler = handler;
}

bool LeavesFlute::ConsoleProc(IThread::Thread* thread, size_t index) {
	OPTICK_THREAD("Console");

#ifndef WIN32
	pollfd cinfd[1];
	cinfd[0].fd = fileno(stdin);
	cinfd[0].events = POLLIN;
#else
	HANDLE h = ::GetStdHandle(STD_INPUT_HANDLE);
#endif
	printf("=> ");
	fflush(stdout);
	while (true) {
#ifndef WIN32
		int ret = poll(cinfd, 1, 1000);
		if (ret > 0 && cinfd[0].revents == POLLIN) {
			String command;
			getline(std::cin, command);
			if (command[command.size() - 1] == '\n') {
				command = command.substr(0, command.size() - 1);
			}

			// Process Command
			if (!command.empty()) {
				IScript::Request& request = interfaces.script.GetDefaultRequest();
				// remove uncessary spaces
				if (!ProcessCommand(String(command.c_str(), command.length()))) {
					break;
				}
			}
			printf("=> ");
			fflush(stdout);
		} else if (ret < 0){
			break;
		}
#else
		DWORD ret = ::WaitForSingleObject(h, INFINITE);

		if (ret == WAIT_OBJECT_0) {
			const size_t CMD_SIZE = 1024;
			static WCHAR buf[CMD_SIZE];
			DWORD read;
			if (ReadConsoleSafe(buf, CMD_SIZE, &read, h)) {
				String unicode((const char*)buf, sizeof(buf[0]) * read);
				// Process Command
				if (buf[0] != '\0') {
					String command = SystemToUtf8(unicode);
					// remove tail spaces and returns
					int32_t end = (int32_t)command.size() - 1;
					while (end >= 0) {
						char ch = command[end];
						if (ch == '\r' || ch == '\n' || ch == '\t' || ch == ' ') {
							end--;
						} else {
							break;
						}
					}

					if (!ProcessCommand(command.substr(0, end + 1)))
						break;
				}
				printf("=> ");
			} else {
				break;
			}
		} else if (ret == WAIT_FAILED) {
			break;
		}
#endif
	}

	return false;
}

size_t LeavesFlute::RequestGetProcessorBitWidth(IScript::Request& request) {
	return sizeof(size_t) * 8;
}

void LeavesFlute::RequestSetScreenSize(IScript::Request& request, Int2& size) {
	interfaces.frame.SetWindowSize(size);
}

Int2 LeavesFlute::RequestGetScreenSize(IScript::Request& request) {
	return interfaces.frame.GetWindowSize();
}

void LeavesFlute::RequestWarpCursor(IScript::Request& request, Int2 position) {
	interfaces.frame.WarpCursor(position);
}

void LeavesFlute::RequestShowCursor(IScript::Request& request, const String& type) {
	IFrame::CURSOR cursor = IFrame::ARROW;
	if (type == "None") {
		cursor = IFrame::NONE;
	} else if (type == "Arrow") {
		cursor = IFrame::ARROW;
	} else if (type == "Cross") {
		cursor = IFrame::CROSS;
	} else if (type == "Wait") {
		cursor = IFrame::WAIT;
	}

	interfaces.frame.ShowCursor(cursor);
}

void LeavesFlute::RequestSetAppTitle(IScript::Request& request, const String& title) {
	DoLock();
	newAppTitle = title;
	UnLock();
}

Interfaces& LeavesFlute::GetInterfaces() const {
	return interfaces;
}

Kernel& LeavesFlute::GetKernel() {
	return bridgeSunset.GetKernel();
}

void LeavesFlute::Require(IScript::Request& request) {
	OPTICK_EVENT();

	// disable extra libs
	// request << global << key("package") << nil << endtable;
	request << global << key("io") << nil << endtable;
	// request << global << key("debug") << nil << endtable;
	request << global << key("os") << nil << endtable;

#ifdef _DEBUG
	request << global << key("DEBUG") << true << endtable;
#else
	request << global << key("DEBUG") << false << endtable;
#endif

#ifdef _WIN32
	request << global << key("DLL") << String(".dll") << endtable;
#else
	request << global << key("DLL") << String(".so") << endtable;
#endif

#if defined(_WIN32)
	char ver[256];
	sprintf(ver, "MSVC %d", _MSC_VER);
	request << global << key("Compiler") << String(ver) << endtable;
#elif defined(__VERSION__)
	request << global << key("Compiler") << String(__VERSION__) << endtable;
#endif

	request.GetScript()->SetErrorHandler(Wrap(this, &LeavesFlute::RequestPrint));

	// takeover the default print call and redirect it to our console.
	request << global << key("System");
	Library::Require(request);
	request << endtable;
}

void LeavesFlute::RequestExit(IScript::Request& request) {
	if (consoleThread != nullptr) {
#ifndef WIN32
		raise(SIGINT);
#else
		FreeConsole();
		::Sleep(100);
#endif
	}

	interfaces.frame.ExitMainLoop();
}

void LeavesFlute::RequestListenConsole(IScript::Request& request, IScript::Request::Ref ref) {
	if (ref) {
		CHECK_REFERENCES_WITH_TYPE(ref, IScript::Request::FUNCTION);
	}

	if (listenConsole) {
		request.DoLock();
		request.Dereference(listenConsole);
		request.UnLock();
	}

	listenConsole = ref;
}

void LeavesFlute::RequestPrint(IScript::Request& request, const String& text) {
	Print(text);
}

void LeavesFlute::OnConsoleOutput(const String& text) {
	DoLock();
	Print(text);
	UnLock();
}

void LeavesFlute::Print(const String& str) {
	// convert utf8 to system encoding
	String text = Utf8ToSystem(str + "\n");
#if defined(_WIN32) || defined(WIN32)
	// wprintf(L"%s\n", text.c_str());
	if (rawPrint) {
		fwprintf(stdout, L"%s", (WCHAR*)text.c_str());
	} else {
		static HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD ws;
		::WriteConsoleW(handle, text.c_str(), (DWORD)wcslen((const WCHAR*)text.c_str()), &ws, nullptr);
	}
#else
	printf("%s", text.c_str());
#endif
}

void LeavesFlute::OnWindowSize(const IFrame::EventSize& size) {
	OPTICK_EVENT();
	interfaces.render.SetDeviceResolution(snowyStream.GetRenderResourceManager()->GetRenderDevice(), size.size);
	mythForest.OnSize(size.size);
}

void LeavesFlute::OnMouse(const IFrame::EventMouse& mouse) {
	OPTICK_EVENT();
	mythForest.OnMouse(mouse);
}

void LeavesFlute::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	OPTICK_EVENT();
	mythForest.OnKeyboard(keyboard);
}

bool LeavesFlute::OnRender() {
	OPTICK_CATEGORY("Render", Optick::Category::Rendering);

	bool titleChanged = false;
	DoLock();
	if (!(appTitle == newAppTitle)) {
		appTitle = newAppTitle;
		titleChanged = true;
	}
	UnLock();

	if (titleChanged) {
		interfaces.frame.SetWindowTitle(appTitle);
	}

	Int2 size = interfaces.frame.GetWindowSize();
	for (size_t i = 0; i < modules.size(); i++) {
		modules[i]->TickDevice(interfaces.render);
	}

	return interfaces.render.NextDeviceFrame(snowyStream.GetRenderResourceManager()->GetRenderDevice());
}

class ExpandParamsScriptTask : public WarpTiny, public TaskRepeat {
public:
	ExpandParamsScriptTask(Kernel& k, const String& p, const std::vector<String>& params, Interfaces& inters) : kernel(k), path(p), value(params), interfaces(inters) {}

	bool LoadScriptText(const String& path, String& text) {
		OPTICK_EVENT();

		uint64_t length = 0;
		bool ret = false;
		IStreamBase* stream = interfaces.archive.Open(path + "." + interfaces.script.GetFileExt(), false, length);
		if (stream == nullptr) {
			stream = interfaces.archive.Open(path, false, length);
		}

		if (stream != nullptr) {
			if (length != 0) {
				// read string
				size_t len = verify_cast<size_t>(length);
				text.resize(len);
				if (stream->Read(const_cast<char*>(text.data()), len)) {
					ret = true;
				}
			} else {
				ret = true;
			}

			stream->Destroy();
		}

		return ret;
	}

	void Abort(void* context) override {
		ReleaseObject();
	}

	void Execute(void* context) override {
		OPTICK_EVENT();

		BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		String text;
		if (LoadScriptText(path, text)) {
			request.DoLock();
			IScript::Request::Ref ref = request.Load(text, path);
			if (ref) {
				request.Push();
				for (size_t i = 0; i < value.size(); i++) {
					request << value[i];
				}
				request.Call(ref);
				request.Dereference(ref);
				printf("=> ");
				request.Pop();
			}

			request.UnLock();
		}

		bridgeSunset.requestPool.ReleaseSafe(&request);
		ReleaseObject();
	}

	Kernel& kernel;
	String path;
	Interfaces& interfaces;
	std::vector<String> value;
};

void LeavesFlute::Execute(const String& path, const std::vector<String>& params) {
	Kernel& kernel = bridgeSunset.GetKernel();
	ExpandParamsScriptTask* task = new ExpandParamsScriptTask(kernel, path, params, interfaces);
	uint32_t warpIndex = task->GetWarpIndex();
	ThreadPool& threadPool = kernel.GetThreadPool();
	kernel.QueueRoutine(task, task);
}

#ifdef _MSC_VER
#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

#ifdef __GNUC__
#define _stricmp strcasecmp
#endif

void LeavesFlute::RequestSearchMemory(IScript::Request& request, const String& memory, size_t start, size_t end, uint32_t alignment, uint32_t maxResult) {
	OPTICK_EVENT();
	bridgeSunset.GetKernel().YieldCurrentWarp();
#ifdef _MSC_VER
	SYSTEM_INFO systemInfo;
	::GetSystemInfo(&systemInfo);
	assert((start & (alignment - 1)) == 0);
	size_t pageSize = systemInfo.dwPageSize;
	uint32_t count = 0;
	std::vector<size_t> addresses;

	for (size_t addr = start; addr < end; addr += pageSize) {
		MEMORY_BASIC_INFORMATION mbi;
		if (::VirtualQuery((LPVOID)addr, &mbi, sizeof(mbi)) != 0) {
			size_t regionEnd = Math::Min(end, (size_t)mbi.BaseAddress + (size_t)mbi.RegionSize);
			if ((mbi.State & MEM_COMMIT) && !(mbi.Protect & (PAGE_WRITECOMBINE | PAGE_NOCACHE | PAGE_GUARD))) {
				for (size_t p = addr; p < regionEnd - memory.size(); p += alignment) {
					if (memcmp((const void*)p, memory.data(), memory.size()) == 0) {
						addresses.emplace_back(p);

						if (maxResult != 0 && ++count >= maxResult) {
							addr = end;
							break;
						}
					}
				}
			}

			addr = (regionEnd + pageSize - 1) & ~(pageSize - 1);
		}
	}

	request.DoLock();
	request << addresses;
	request.UnLock();
#endif
}

String LeavesFlute::RequestGetSystemPlatform(IScript::Request& request) {
#if defined(_WIN32)
	return "Windows";
#elif defined(__linux__)
	return "Linux";
#elif defined(__APPLE__)
	return "MacOS/iOS";
#else
	return "Unix"; // maybe
#endif
}

size_t LeavesFlute::RequestLoadLibrary(IScript::Request& request, const String& path) {
	String last = path.substr(path.length() - 3, 3);
#if defined(__linux__)
	if (_stricmp(last.c_str(), ".so") == 0) {
		return (size_t)dlopen(Utf8ToSystem(path).c_str(), RTLD_NOW | RTLD_GLOBAL);
	}
#elif defined(_WIN32)
	if (_stricmp(last.c_str(), "dll") == 0) {
		WCHAR fullPathName[MAX_PATH * 4];
		::GetFullPathNameW((const WCHAR*)Utf8ToSystem(path).c_str(), MAX_PATH * 4, fullPathName, nullptr);
		return (size_t)::LoadLibraryExW(fullPathName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
	}
#endif

	return 0;
}

void LeavesFlute::RequestCallLibrary(IScript::Request& request, size_t handle, const String& entry, IScript::Request::Arguments args) {
	// _cdecl calling convension for compatibility of parameter counts
	typedef void (*SetupProxy)(Interfaces&, IScript::Request&);
	if (handle != 0) {
#if defined(__linux__)
		SetupProxy proxy = (SetupProxy)dlsym((void*)handle, entry.c_str());
#elif defined(_WIN32)
		SetupProxy proxy = (SetupProxy)::GetProcAddress((HMODULE)handle, entry.c_str());
#endif
		if (proxy != nullptr) {
			proxy(GetInterfaces(), request);
		}
	}
}

bool LeavesFlute::RequestFreeLibrary(IScript::Request& request, size_t handle) {
#if defined(__linux__)
	return dlclose((void*)handle) == 0;
#elif defined(_WIN32)
	return ::FreeLibrary((HMODULE)handle) != 0;
#else
	return 0;
#endif
}

TObject<IReflect>& LeavesFlute::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		// dependency order
		ReflectProperty(purpleTrail)[ScriptLibrary = "PurpleTrail"];
		ReflectProperty(bridgeSunset)[ScriptLibrary = "BridgeSunset"];
		ReflectProperty(remembery)[ScriptLibrary = "Remembery"];
		ReflectProperty(echoLegend)[ScriptLibrary = "EchoLegend"];
		ReflectProperty(heartVioliner)[ScriptLibrary = "HeartVioliner"];
		ReflectProperty(snowyStream)[ScriptLibrary = "SnowyStream"];
		ReflectProperty(mythForest)[ScriptLibrary = "MythForest"];
		ReflectProperty(galaxyWeaver)[ScriptLibrary = "GalaxyWeaver"];
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestGetSystemPlatform)[ScriptMethod = "GetSystemPlatform"];
		ReflectMethod(RequestGetProcessorBitWidth)[ScriptMethod = "GetProcessorBitWidth"];
		ReflectMethod(RequestExit)[ScriptMethod = "Exit"];
		ReflectMethod(RequestPrint)[ScriptMethod = "Print"];
		ReflectMethod(RequestSetAppTitle)[ScriptMethod = "SetAppTitle"];
		ReflectMethod(RequestShowCursor)[ScriptMethod = "ShowCursor"];
		ReflectMethod(RequestWarpCursor)[ScriptMethod = "WrapCursor"];
		ReflectMethod(RequestSetScreenSize)[ScriptMethodLocked = "SetScreenSize"];
		ReflectMethod(RequestGetScreenSize)[ScriptMethodLocked = "GetScreenSize"];
		ReflectMethod(RequestListenConsole)[ScriptMethod = "ListenConsole"];
		ReflectMethod(RequestSearchMemory)[ScriptMethod = "SearchMemory"];
		ReflectMethod(RequestLoadLibrary)[ScriptMethod = "LoadLibrary"];
		ReflectMethod(RequestCallLibrary)[ScriptMethod = "CallLibrary"];
		ReflectMethod(RequestFreeLibrary)[ScriptMethod = "FreeLibrary"];
	}

	return *this;
}
