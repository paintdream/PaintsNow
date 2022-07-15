// IDebugger.h
// PaintDream (paintdream@paintdream.com)
// 2015-6-16
//

#pragma once
#include "../../Core/Interface/IType.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Interface/IDevice.h"
#include <string>

namespace PaintsNow {
	class pure_interface IDebugger : public IDevice {
	public:
		~IDebugger() override;
		virtual void SetDumpHandler(const String& path, const TWrapper<bool>& handler) = 0;
		virtual void StartDump(const String& options) = 0;
		virtual void EndDump() = 0;
		virtual void InvokeDump(const String& options) = 0;
	};
}

