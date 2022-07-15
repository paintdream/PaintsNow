#include "../../Utility/LeavesFlute/LeavesFlute.h"
#include "../../Utility/LeavesFlute/Platform.h"
#ifdef _WIN32
#include "../../General/Driver/Debugger/MiniDump/ZDebuggerWin.h"
#include "Helpers/Win32/ServiceWin32.h"
#include <shlwapi.h>
// Tips: How to use VLD to detect memory leaks:
//   1. Use VS 2015 or earlier version, install VLD.
//   2. Do not load RenderDoc.dll [in ZDebuggerRenderDoc::ZDebuggerRenderDoc()].
//   3. Remove the call to 'wglDeleteContext' in function 'destroyContextWGL' of GLFW and ignore its leak.
//      Because this call will destroy CRT memory leak detection!!!
//   4. CMake the solution with USE_OPTICK = 0.
//   5. Uncomment the following line.
// #include <vld.h>
#endif
#include "../../Utility/LeavesFlute/Loader.h"
#include <ctime>

#include "Helpers/ImGui/LeavesImGui.h"

#if USE_LEAVES_IMGUI
#include "Helpers/ImGui/System.h"
#include "Helpers/ImGui/Repository.h"
#include "Helpers/ImGui/RenderFlowGraph.h"
#include "Helpers/ImGui/Script.h"
#include "Helpers/ImGui/IModule.h"
#endif

using namespace PaintsNow;

static bool DumpHandler() {
	// always write minidump file
	return true;
}

int main(int argc, char* argv[]) {
	// Register/unregister service?
#ifdef _WIN32
	WSADATA wsa;
	::WSAStartup(0x201, &wsa);

	ZDebuggerWin dumper;
	time_t t;
	time(&t);
	tm* x = localtime(&t);
	char fileName[256];
	sprintf(fileName, "LeavesWing_%04d_%02d_%02d_%02d_%02d_%02d.dmp", x->tm_year, x->tm_mon, x->tm_mday, x->tm_hour, x->tm_min, x->tm_sec);
	dumper.SetDumpHandler(fileName, &DumpHandler);

	::CoInitializeEx(0, COINIT_MULTITHREADED);
	if (argc > 1) {
		if (strcmp(argv[1], "/install") == 0) {
			if (ServiceWin32::GetInstance().InstallService()) {
				printf("Service installation complete!\n");
				return 0;
			} else {
				printf("Service installation error!\n");
				return -1;
			}
		} else if (strcmp(argv[1], "/uninstall") == 0) {
			if (ServiceWin32::GetInstance().DeleteService()) {
				printf("Service uninstallation complete!\n");
				return 0;
			} else {
				printf("Service uninstallation error!\n");
				return -1;
			}
		} else if (strcmp(argv[1], "/service") == 0) {
			return ServiceWin32::GetInstance().RunServiceWorker(argc, argv) ? 0 : -1;
		}
	}
	
	if (ServiceWin32::InServiceContext()) {
		return ServiceWin32::GetInstance().RunServiceMaster(argc, argv) ? 0 : -1;
	}
#endif

	CmdLine cmdLine;

	// support for drag'n'drop startup profile
#ifdef _WIN32
	if (argc == 2 && argv[1][0] != '-') {
		WCHAR target[MAX_PATH * 2] = L"";
		WCHAR current[MAX_PATH * 2] = L"";
		::GetCurrentDirectoryW(MAX_PATH * 2, current);
		String mount;
		if (::PathRelativePathToW(target, current, FILE_ATTRIBUTE_DIRECTORY, (LPCWSTR)Utf8ToSystem(argv[1]).c_str(), FILE_ATTRIBUTE_NORMAL)) {
			mount = String("--Mount=") + SystemToUtf8(String((const char*)target, wcslen(target) * 2));
		} else {
			mount = String("--Mount=") + argv[1];
		}

		char* dragArgs[] = {
			argv[0],
			"--Graphic=true",
			const_cast<char*>(mount.c_str())
		};

		cmdLine.Process(sizeof(dragArgs) / sizeof(dragArgs[0]), dragArgs);
	} else {
		cmdLine.Process(argc, argv);
	}
#else
	cmdLine.Process(argc, argv);
#endif
	printf("LeavesWing %s\nPaintDream (paintdream@paintdream.com) (C) 2014-2022\nBased on PaintsNow [https://github.com/paintdream/paintsnow]\n", PAINTSNOW_VERSION_MINOR);

	Loader loader;
#if USE_LEAVES_IMGUI
	std::map<String, CmdLine::Option>::const_iterator it = cmdLine.GetFactoryMap().find("IFrame");
	if (it != cmdLine.GetFactoryMap().end() && it->second.name == "ZFrameGLFWImGui") {
		System system;
		IModule visualizer;
		Repository repository;
		Script script;
		RenderFlowGraph renderFlowGraph;

		LeavesImGui leavesImGui(loader.GetLeavesFluteReference());
		TWrapper<IFrame*> frameFactory = WrapFactory(UniqueType<ZFrameGLFWImGui>(), std::ref(leavesImGui));

		loader.GetConfig().RegisterFactory("IFrame", "ZFrameGLFWImGui", frameFactory);
		leavesImGui.AddWidget(&system);
		leavesImGui.AddWidget(&repository);
		leavesImGui.AddWidget(&visualizer);
		// leavesImGui.AddWidget(&script);
		// leavesImGui.AddWidget(&renderFlowGraph);
		loader.Run(cmdLine);
	} else {
		loader.Run(cmdLine);
	}
#else
	loader.Run(cmdLine);
#endif

#ifdef _WIN32
	::WSACleanup();

	OutputDebugStringA("LeavesWing exited without any errors.\n");
	::CoUninitialize();
#endif
	return 0;
}
