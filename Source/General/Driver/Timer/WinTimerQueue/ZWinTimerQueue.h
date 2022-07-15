// ZWinTimerQueue.h]
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once
#include "../../../Interface/ITimer.h"

#if !defined(_WIN32) && !defined(WIN32)
#error "Timer Queue can only be used on Windows System."
#endif

namespace PaintsNow {
	class ZWinTimerQueue final : public ITimer {
	public:
		ZWinTimerQueue();
		virtual ~ZWinTimerQueue();

		virtual Timer* StartTimer(size_t interval, const TWrapper<void, size_t>& wrapper);
		virtual void StopTimer(Timer* timer);
		virtual size_t GetTimerInterval(Timer* timer) const;

	};
}
