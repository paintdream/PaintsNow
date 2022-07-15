// ZDebuggerRenderDoc.h
// PaintDream (paintdream@paintdream.com)
// 2020-4-4
//

#pragma once
#include "../../../Interface/IDebugger.h"

struct RENDERDOC_API_1_4_0;

namespace PaintsNow {
	class ZDebuggerRenderDoc : public IDebugger {
	public:
		ZDebuggerRenderDoc();
		void SetDumpHandler(const String& path, const TWrapper<bool>& handler) override;
		void StartDump(const String& options) override;
		void EndDump() override;
		void InvokeDump(const String& options) override;

	private:
		String dumpPath;
		TWrapper<bool> dumpHandler;
		RENDERDOC_API_1_4_0* api;
	};
}

