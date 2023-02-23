#include "LeavesFlute.h"
#include "../SnowyStream/Manager/RenderResourceManager.h"
#include "../../Core/Driver/Filter/Pod/ZFilterPod.h"
#include "../../General/Driver/Filter/Json/ZFilterJson.h"
#include "../../General/Driver/Filter/LZMA/ZFilterLZMA.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"
#include "Platform.h"
#include <iostream>
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#include <dlfcn.h>
#endif

using namespace PaintsNow;

class ScanModules : public IReflect {
public:
	ScanModules(std::unordered_map<String, void*>& smap, std::vector<IScript::Library*>& m) : IReflect(true, false), symbolMap(smap), modules(m) {}
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (!s.IsBasicObject() && s.QueryInterface(UniqueType<IScript::Library>()) != nullptr) {
			IScript::Library* library = static_cast<IScript::Library*>(&s);
			modules.emplace_back(library);
			symbolMap[name] = library;
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	std::unordered_map<String, void*>& symbolMap;
	std::vector<IScript::Library*>& modules;
};

LeavesFlute::LeavesFlute(bool ng, Interfaces& inters, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const String& defMount, uint32_t threadCount, uint32_t warpCount, long balancer) :
	ISyncObject(inters.thread),
	interfaces(inters),
	iceFlow(),
	purpleTrail(),
	bridgeSunset(inters.thread, inters.script, threadCount, warpCount, balancer),
	echoLegend(inters.thread, inters.standardNetwork, inters.quickNetwork, bridgeSunset),
	snowyStream(inters, bridgeSunset, subArchiveCreator, defMount),
	mythForest(inters, snowyStream, bridgeSunset),
	heartVioliner(inters.thread, inters.timer, bridgeSunset),
	remembery(inters.thread, inters.archive, inters.database, bridgeSunset),
	galaxyWeaver(inters.thread, inters.standardNetwork, bridgeSunset, snowyStream, mythForest),
	crossScriptModule(bridgeSunset.GetKernel().GetThreadPool(), inters.script) {
#ifdef _WIN32
	::SetConsoleOutputCP(65001); // UTF8
	consoleThreadID = 0;

#if defined(_MSC_VER) && _MSC_VER > 1200
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 0;                   // Width of each character in the font
	cfi.dwFontSize.Y = 24;                  // Height
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	std::wcscpy(cfi.FaceName, L"Consolas"); // Choose your font
	::SetCurrentConsoleFontEx(::GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
#endif
	//setvbuf(stdout, nullptr, _IOFBF, 1000);
#endif
	looping.store(0, std::memory_order_relaxed);

	ScanModules scanModules(symbolMap, modules);
	(*this)(scanModules);

	symbolMap["UniqueAllocator"] = &UniqueAllocator::GetInstance();
	symbolMap["Kernel"] = &bridgeSunset.GetKernel();
	symbolMap["Interfaces"] = &interfaces;
	symbolMap["Plugin"] = static_cast<IPlugin*>(this);

	Initialize();

	bridgeSunset.LogInfo().Printf("[LeavesFlute] Initialized ...\n");

	interfaces.frame.SetCallback(this);
	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	ScriptRequire(request); // register callbacks
	request.UnLock();
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
	std::vector<EngineCallback*> callbacks;

	DoLock();
	callbacks = engineCallbacks;
	UnLock();

	for (size_t i = 0; i < callbacks.size(); i++) {
		callbacks[i]->OnEngineReset(true);
	}

	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	if (listenConsole) {
		request.Dereference(listenConsole);
	}
	request.UnLock();
	bridgeSunset.Reset();
	interfaces.script.Reset();

	Uninitialize();
	bridgeSunset.LogInfo().Printf("[LeavesFlute] Uninitialized ...\n");
}

void LeavesFlute::Setup() {
	if (setupHandler) {
		setupHandler(*this);
	}
}

void LeavesFlute::EnterStdinLoop() {
	bridgeSunset.LogInfo().Printf("[LeavesFlute] Enter Standard Console Environment ...\n");

	looping.store(1, std::memory_order_release);

	if (consoleHandler) {
		consoleHandler(*this);
	} else {
		ConsoleProc(nullptr, 0);
	}

	looping.store(0, std::memory_order_release);
}

#ifdef _WIN32
static void CALLBACK Papcfunc(ULONG_PTR Parameter) {}
#endif

void LeavesFlute::EnterMainLoop() {
	bridgeSunset.LogInfo().Printf("[LeavesFlute] Enter Standard Graphic Environment ...\n");

	assert(!consoleHandler);
	IThread::Thread* consoleThread = interfaces.thread.NewThread(Wrap(this, &LeavesFlute::ConsoleProc), 0);
	interfaces.thread.SetThreadName(consoleThread, "Console Thread");

	looping.store(1, std::memory_order_release);
	interfaces.frame.EnterMainLoop();
	looping.store(0, std::memory_order_release);
	bridgeSunset.LogInfo().Printf("[LeavesFlute] Exit Standard Graphic Environment ...\n");
#ifdef _WIN32
	if (consoleThreadID != 0)
	{
		HANDLE handle = ::OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)consoleThreadID);
		if (handle != nullptr)
		{
			::QueueUserAPC(Papcfunc, handle, 0);
			::CloseHandle(handle);
			::FreeConsole();
			threadApi.Wait(consoleThread);
		}
	}
#endif
	threadApi.DeleteThread(consoleThread);
}

#ifdef WIN32

bool ReadConsoleSafe(WCHAR buf[], DWORD size, DWORD* read, HANDLE h) {
	// may crash or memory leak when user close the window on win7
	// but why ?
	__try {
		if (::ReadConsoleW(h, buf, size - 1, read, nullptr)) {
			return true;
		}
	} __except (EXCEPTION_ACCESS_VIOLATION) {
		fprintf(stderr, "Unexpected access violation\n");
	}

	return false;
}

#else
#include <sys/poll.h>
#endif

bool LeavesFlute::ProcessCommand(const String& command) {
	if (command == "quit" || command == "exit") {
		return false;
	} else {
		IScript::Request& request = interfaces.script.GetDefaultRequest();
		if (!listenConsole) {
			request.DoLock();
			IScript::Request::Ref code = request.Load(command, "Console");
			request.UnLock();

			if (code) {
				bridgeSunset.GetThreadPool().Dispatch(CreateTaskScriptOnce(code));
			}
		} else {
			bridgeSunset.GetThreadPool().Dispatch(CreateTaskScript(listenConsole, command));
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

bool LeavesFlute::ConsoleProc(IThread::Thread* thread, size_t handle) {
	OPTICK_THREAD("Console");

#ifndef WIN32
	pollfd cinfd[1];
	cinfd[0].fd = fileno(stdin);
	cinfd[0].events = POLLIN;
#else
	HANDLE h = ::GetStdHandle(STD_INPUT_HANDLE);
	consoleThreadID = ::GetCurrentThreadId();
	
	const size_t CMD_SIZE = 1024;
	WCHAR buf[CMD_SIZE];
#endif
	while (looping.load(std::memory_order_acquire) != 0) {
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
		} else if (ret < 0) {
			break;
		}
#else
		DWORD ret = ::WaitForSingleObjectEx(h, INFINITE, TRUE);

		if (ret == WAIT_OBJECT_0) {
			DWORD read;
			if (ReadConsoleSafe(buf, CMD_SIZE, &read, h) && read != 0) {
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
			} else {
				break;
			}
		} else if (ret == WAIT_OBJECT_0 + 1 || ret == WAIT_FAILED || ret == WAIT_IO_COMPLETION) {
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

void LeavesFlute::ScriptRequire(IScript::Request& request) {
	OPTICK_EVENT();

	/*
	request.Push();
	Float3 v1(1, 2, 3);
	Float3 v2(2, 3, 4);
	request << IScript::ManageObject(v1);
	request << IScript::ManageObject(std::move(v2));
	IScript::TManagedObject<Float3> m2(v2);
	request << m2.Ref();
	request >> IScript::ManageReference(v2);
	request.Pop();*/

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
	Library::ScriptRequire(request);
	request << endtable;
}

void LeavesFlute::Exit() {
	interfaces.frame.ExitMainLoop();
}

void LeavesFlute::RequestExit(IScript::Request& request) {
	bridgeSunset.LogInfo().Printf("[Script] Exit requested.\n");
	Exit();
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
	bridgeSunset.LogInfo().Printf("#FFFF00#[Script] %s\n", text.c_str());
}

void LeavesFlute::OnWindowSize(const IFrame::EventSize& size) {
	OPTICK_EVENT();

	if (size.size.x() != 0 && size.size.y() != 0) {
		interfaces.render.SetDeviceResolution(snowyStream.GetRenderResourceManager()->GetRenderDevice(), size.size);
		mythForest.OnSize(size.size);
	}
}

void LeavesFlute::OnMouse(const IFrame::EventMouse& mouse) {
	OPTICK_EVENT();
	mythForest.OnMouse(mouse);
}

void LeavesFlute::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	OPTICK_EVENT();
	mythForest.OnKeyboard(keyboard);
}

void LeavesFlute::OnRender() {
	OPTICK_CATEGORY("Render", Optick::Category::Rendering);
	IRender& render = interfaces.render;
	IRender::Device* device = snowyStream.GetRenderResourceManager()->GetRenderDevice();

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

	if (render.PreDeviceFrame(device)) {
		std::vector<EngineCallback*> callbacks;

		DoLock();
		callbacks = engineCallbacks;
		UnLock();

		for (size_t i = 0; i < callbacks.size(); i++) {
			callbacks[i]->OnRenderBegin();
		}

		Int2 size = interfaces.frame.GetWindowSize();
		for (size_t k = 0; k < modules.size(); k++) {
			modules[k]->TickDevice(interfaces.render);
		}

		DoLock();
		callbacks = engineCallbacks;
		UnLock();

		for (size_t n = callbacks.size(); n > 0; n--) {
			callbacks[n - 1]->OnRenderEnd();
		}

		render.PostDeviceFrame(device);
	}
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
	WarpYieldGuard guard(bridgeSunset.GetKernel());
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
	bridgeSunset.LogInfo().Printf("[LeavesFlute] LoadLibrary: %s\n", path.c_str());

	String last = path.substr(path.length() - 3, 3);
	typedef bool (*LeavesMain)(IPlugin* plugin);
#if defined(__linux__)
	if (_stricmp(last.c_str(), ".so") == 0) {
		void* m = dlopen(Utf8ToSystem(path).c_str(), RTLD_NOW | RTLD_GLOBAL);
		if (m != nullptr) {
			LeavesMain leavesMain = (LeavesMain)::dlsym(m, "LeavesMain");
			if (leavesMain != nullptr && !leavesMain(this)) {
				dlclose(m);
				return 0;
			}
		}

		return (size_t)m;
	}
#elif defined(_WIN32)
	if (_stricmp(last.c_str(), "dll") == 0) {
		WCHAR fullPathName[MAX_PATH * 4];
		::GetFullPathNameW((const WCHAR*)Utf8ToSystem(path).c_str(), MAX_PATH * 4, fullPathName, nullptr);
		HMODULE m = ::LoadLibraryExW(fullPathName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (m != nullptr) {
			LeavesMain leavesMain = (LeavesMain)::GetProcAddress(m, "LeavesMain");
			if (leavesMain != nullptr && !leavesMain(this)) {
				::FreeLibrary(m);
				return 0;
			}
		}

		return (size_t)m;
	}
#endif

	return 0;
}

size_t LeavesFlute::RequestGetSymbolAddress(IScript::Request& request, size_t handle, const String& entry) {
	if (handle == 0) {
		std::unordered_map<String, void*>::iterator it = symbolMap.find(entry);
		return it != symbolMap.end() ? (size_t)(*it).second : 0;
	} else {
#if defined(__linux__)
		return (size_t)(void*)dlsym((void*)handle, entry.c_str());
#elif defined(_WIN32)
		return (size_t)(void*)::GetProcAddress((HMODULE)handle, entry.c_str());
#else
		return 0;
#endif
	}
}

size_t LeavesFlute::RequestCallSymbol(IScript::Request& request, size_t addressValue, IScript::Request::Arguments args) {
	size_t ret = 0;
	if (addressValue != 0) {
		void* address = reinterpret_cast<void*>(addressValue);
		const size_t MAX_PARAM_COUNT = 8;
		size_t params[MAX_PARAM_COUNT] = { 0 };
		for (size_t i = 0; i < args.count; i++)
		{
			if (request.GetCurrentType() == IScript::Request::STRING)
			{
				request >> reinterpret_cast<const char*&>(params[i]);
			}
			else
			{
				request >> params[i];
			}
		}

		request.UnLock();

		switch (args.count)
		{
		case 0:
			ret = ((size_t(*)())address)();
			break;
		case 1:
			ret = ((size_t(*)(size_t))address)(params[0]);
			break;
		case 2:
			ret = ((size_t(*)(size_t, size_t))address)(params[0], params[1]);
			break;
		case 3:
			ret = ((size_t(*)(size_t, size_t, size_t))address)(params[0], params[1], params[2]);
			break;
		case 4:
			ret = ((size_t(*)(size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3]);
			break;
		case 5:
			ret = ((size_t(*)(size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4]);
			break;
		case 6:
			ret = ((size_t(*)(size_t, size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4], params[5]);
			break;
		case 7:
			ret = ((size_t(*)(size_t, size_t, size_t, size_t, size_t, size_t, size_t))address)(params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
			break;
		}

		request.DoLock();
	}

	return ret;
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
		ReflectProperty(bridgeSunset)[ScriptLibrary = "BridgeSunset"];
		ReflectProperty(purpleTrail)[ScriptLibrary = "PurpleTrail"];
		ReflectProperty(iceFlow)[ScriptLibrary = "IceFlow"];
		ReflectProperty(remembery)[ScriptLibrary = "Remembery"];
		ReflectProperty(echoLegend)[ScriptLibrary = "EchoLegend"];
		ReflectProperty(heartVioliner)[ScriptLibrary = "HeartVioliner"];
		ReflectProperty(snowyStream)[ScriptLibrary = "SnowyStream"];
		ReflectProperty(mythForest)[ScriptLibrary = "MythForest"];
		ReflectProperty(galaxyWeaver)[ScriptLibrary = "GalaxyWeaver"];
		ReflectProperty(crossScriptModule)[ScriptLibrary = "CrossScriptModule"];
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
		ReflectMethod(RequestGetSymbolAddress)[ScriptMethod = "GetSymbolAddress"];
		ReflectMethod(RequestCallSymbol)[ScriptMethod = "CallSymbol"];
		ReflectMethod(RequestFreeLibrary)[ScriptMethod = "FreeLibrary"];
	}

	return *this;
}

// Plugin
class_aligned(CPU_CACHELINE_SIZE) PluginTask : public WarpTiny, public TaskOnce {
public:
	PluginTask(IPlugin::Task* t) : task(t) {}
	void Execute(void* context) override {
		task->Execute();
		ITask::Delete(this);
	}

	void Abort(void* context) override {
		task->Abort();
		ITask::Delete(this);
	}

	IPlugin::Task* task;
};

unsigned int LeavesFlute::GetThreadCount() {
	return bridgeSunset.GetThreadPool().GetThreadCount();
}

unsigned int LeavesFlute::GetCurrentThreadIndex() {
	return bridgeSunset.GetThreadPool().GetCurrentThreadIndex();
}

void* LeavesFlute::AllocateMemory(unsigned int size) {
	return ITask::Allocate(size);
}

void LeavesFlute::FreeMemory(void* p, unsigned int size) {
	ITask::Deallocate(p, size);
}

void LeavesFlute::QueueTask(Task* task, int priority) {
	PluginTask* pluginTask = new (ITask::Allocate(sizeof(PluginTask))) PluginTask(task);
	bridgeSunset.GetThreadPool().Dispatch(pluginTask, priority);
}

void LeavesFlute::QueueWarpTask(Task* task, unsigned int warp) {
	PluginTask* pluginTask = new (ITask::Allocate(sizeof(PluginTask))) PluginTask(task);
	pluginTask->SetWarpIndex(warp);
	bridgeSunset.GetKernel().QueueRoutine(pluginTask, pluginTask);
}

unsigned int LeavesFlute::GetCurrentWarpIndex() {
	return bridgeSunset.GetKernel().GetCurrentWarpIndex();
}
void LeavesFlute::SetWarpPriority(unsigned int warp, int priority) {
	bridgeSunset.GetKernel().SetWarpPriority(warp, priority);
}

unsigned int LeavesFlute::YieldCurrentWarp() {
	return bridgeSunset.GetKernel().YieldCurrentWarp();
}

void LeavesFlute::SuspendWarp(unsigned int warp) {
	bridgeSunset.GetKernel().SuspendWarp(warp);
}

void LeavesFlute::ResumeWarp(unsigned int warp) {
	bridgeSunset.GetKernel().ResumeWarp(warp);
}

unsigned int LeavesFlute::AllocateWarpIndex() {
	return bridgeSunset.AllocateWarpIndex();
}

void LeavesFlute::FreeWarpIndex(unsigned int warp) {
	bridgeSunset.FreeWarpIndex(warp);
}

void LeavesFlute::RegisterEngineCallback(EngineCallback* frameCallback) {
	DoLock();
	BinaryInsert(engineCallbacks, frameCallback);
	UnLock();
}

void LeavesFlute::UnregisterEngineCallback(EngineCallback* frameCallback) {
	DoLock();
	BinaryErase(engineCallbacks, frameCallback);
	UnLock();
}

IPlugin::Stream* LeavesFlute::OpenStream(const char* path, bool openExisting, unsigned long& len) {
	IArchive& archive = interfaces.archive;
	uint64_t length;
	IStreamBase* stream = archive.Open(path, !openExisting, length);
	if (stream == nullptr) {
		return nullptr;
	}

	if (length > 0xffffffff) {
		stream->Destroy();
		return nullptr;
	}

	return reinterpret_cast<IPlugin::Stream*>(stream);
}

void LeavesFlute::FlushStream(Stream* stream) {
	assert(stream != nullptr);
	IStreamBase* s = reinterpret_cast<IStreamBase*>(stream);

	s->Flush();
}

bool LeavesFlute::ReadStream(Stream* stream, void* p, unsigned long& len) {
	assert(stream != nullptr);
	IStreamBase* s = reinterpret_cast<IStreamBase*>(stream);
	size_t l = 0;
	
	return s->Read(p, l) && (len = verify_cast<unsigned long>(l)) != 0;
}

bool LeavesFlute::WriteStream(Stream* stream, const void* p, unsigned long& len) {
	assert(stream != nullptr);
	IStreamBase* s = reinterpret_cast<IStreamBase*>(stream);
	size_t l = len;
	
	return s->Write(p, l) && (len = verify_cast<unsigned long>(l)) != 0;
}

bool LeavesFlute::SeekStream(Stream* stream, SEEK_OPTION option, long offset) {
	assert(stream != nullptr);
	IStreamBase* s = reinterpret_cast<IStreamBase*>(stream);
	return s->Seek((IStreamBase::SEEK_OPTION)option, offset);
}

void LeavesFlute::CloseStream(Stream* stream) {
	assert(stream != nullptr);
	IStreamBase* s = reinterpret_cast<IStreamBase*>(stream);
	s->Destroy();
}

void LeavesFlute::PushProfilerSection(const char* text) {
	OPTICK_PUSH_DYNAMIC(text);
}

void LeavesFlute::PopProfilerSection() {
	OPTICK_POP();
}

void* LeavesFlute::GetSymbolAddress(const char* entry) {
	std::unordered_map<String, void*>::iterator it = symbolMap.find(entry);
	return it != symbolMap.end() ? (*it).second : 0;
}

const char* LeavesFlute::GetVersionMajor() {
	return PAINTSNOW_VERSION_MAJOR;
}

const char* LeavesFlute::GetVersionMinor() {
	return PAINTSNOW_VERSION_MINOR;
}

class PluginScriptHandler {
public:
	String Call(IScript::Request& request, StringView data) {
		unsigned long len = verify_cast<unsigned long>(data.length());
		const char* ret = scriptHandler(data.data(), len, context);
		if (ret != nullptr) {
			String response(ret, len);
			scriptHandlerResponse(ret, len, context);
			return response;
		} else {
			return String();
		}
	}

	IPlugin::ScriptHandler scriptHandler;
	IPlugin::ScriptHandlerResponse scriptHandlerResponse;
	void* context;
};

IPlugin::Script* LeavesFlute::AllocateScript() {
	TShared<CrossScript> crossScript = crossScriptModule.New(false, false);
	crossScript->ReferenceObject();
	return reinterpret_cast<IPlugin::Script*>(crossScript());
}

void LeavesFlute::FreeScript(IPlugin::Script* script) {
	assert(script != nullptr);
	CrossScript* crossScript = reinterpret_cast<CrossScript*>(script);
	crossScript->ReleaseObject();
}

void LeavesFlute::RegisterScriptHandler(Script* script, const char* procedure, ScriptHandler scriptHandler, ScriptHandlerResponse finalizer, void* context) {
	assert(procedure != nullptr);
	assert(scriptHandler != nullptr);

	IScript::Request& request = script == nullptr ? interfaces.script.GetDefaultRequest() : reinterpret_cast<CrossScript*>(script)->GetScript().GetDefaultRequest();

	PluginScriptHandler handler;
	handler.scriptHandler = scriptHandler;
	handler.scriptHandlerResponse = finalizer;
	handler.context = context;

	request.DoLock();
	request << global << key(procedure) << request.Adapt(WrapClosure(std::move(handler), &PluginScriptHandler::Call)) << endtable;
	request.UnLock();
}

void LeavesFlute::UnregisterScriptHandler(Script* script, const char* procedure) {
	assert(procedure != nullptr);
	IScript::Request& request = script == nullptr ? interfaces.script.GetDefaultRequest() : reinterpret_cast<CrossScript*>(script)->GetScript().GetDefaultRequest();

	request.DoLock();
	request << global << key(procedure) << nil << endtable;
	request.UnLock();
}

bool LeavesFlute::CallScript(Script* script, const char* procedure, const char* requestData, unsigned long len, ScriptHandlerResponse response, void* context) {
	assert(procedure != nullptr);
	IScript::Request* req = script == nullptr ? bridgeSunset.allocate(1) : reinterpret_cast<CrossScript*>(script)->allocate(1);
	bool retValue = false;

	if (req != nullptr) {
		IScript::Request& request = *req;

		request.DoLock();
		IScript::Request::Ref func;
		request.Push();
		request << global << key(procedure) >> func << endtable;
		if (func) {
			StringView ret;
			if (request.Call(func, StringView(requestData, (size_t)len))) {
				request >> ret;
				response(ret.data(), (long)ret.length(), context);
				retValue = true;
			}
		}
		request.Pop();
		request.UnLock();

		if (script == nullptr) {
			bridgeSunset.deallocate(req, 1);
		} else {
			reinterpret_cast<CrossScript*>(script)->deallocate(req, 1);
		}
	}

	return retValue;
}

