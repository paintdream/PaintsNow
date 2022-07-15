// ZPosixTimer.h]
// By PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma oncex

#include "../../../Interface/ITimer.h"

#include <cstdlib>
#include <signal.h>
#include <unistd.h>

namespace PaintsNow {
	class ZPosixTimer final : public ITimer {
	public:
		ZPosixTimer();
		~ZPosixTimer() override;

		Timer* StartTimer(size_t interval, const TWrapper<void, size_t>& wrapper) override;
		void StopTimer(Timer* timer) override;
		size_t GetTimerInterval(Timer* ) const override;

	};
}
