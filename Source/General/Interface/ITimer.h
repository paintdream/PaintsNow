// ITimer.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Interface/IDevice.h"

namespace PaintsNow {
	class pure_interface ITimer : public IDevice {
	public:
		~ITimer() override;
		struct Timer {};
		virtual Timer* StartTimer(size_t interval, const TWrapper<void, size_t>& wrapper) = 0;
		virtual void StopTimer(Timer* timer) = 0;
		virtual size_t GetTimerInterval(Timer* timer) const = 0;

		static int64_t GetSystemClock();
	};
}

