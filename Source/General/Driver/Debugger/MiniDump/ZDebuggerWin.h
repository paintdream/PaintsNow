// ZDebuggerWin.h
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../Interface/IDebugger.h"

namespace PaintsNow {
	class ZDebuggerWin final : public IDebugger {
	public:
		ZDebuggerWin();
		virtual ~ZDebuggerWin();
		void SetDumpHandler(const String& path, const TWrapper<bool>& handler) override;
		void StartDump(const String& options) override;
		void EndDump() override;
		void InvokeDump(const String& options) override;

		TWrapper<bool> handler;
		String path;
	};
}
