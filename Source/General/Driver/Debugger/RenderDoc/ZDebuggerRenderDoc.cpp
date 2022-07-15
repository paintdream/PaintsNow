#include "ZDebuggerRenderDoc.h"
#if defined(_MSC_VER) && _MSC_VER <= 1200
#define RENDERDOC_NO_STDINT
#include "../../../../Core/Interface/IType.h"
#endif
#include "renderdoc_app.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

using namespace PaintsNow;

ZDebuggerRenderDoc::ZDebuggerRenderDoc() : api(nullptr) {
#ifdef _WIN32
	HMODULE mod = ::LoadLibraryW(L"renderdoc.dll");
	if (mod != nullptr) {
		pRENDERDOC_GetAPI RENDERDOC_GetAPI =
			(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_0, (void **)&api);
		assert(ret == 1);
	}
#else
	// At init, on linux/android.
	// For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
	if (void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&api);
		assert(ret == 1);
	}
#endif
}

void ZDebuggerRenderDoc::SetDumpHandler(const String& path, const TWrapper<bool>& handler) {
	dumpPath = path;
	dumpHandler = handler;
}

void ZDebuggerRenderDoc::StartDump(const String& options) {
	if (api != nullptr) {
		api->SetCaptureFilePathTemplate(dumpPath.c_str());
		api->StartFrameCapture(nullptr, nullptr);
	}
}

void ZDebuggerRenderDoc::EndDump() {
	if (api != nullptr) {
		api->EndFrameCapture(nullptr, nullptr);
	}
}

void ZDebuggerRenderDoc::InvokeDump(const String& options) {
	if (api != nullptr) {
		api->TriggerCapture();
	}
}
