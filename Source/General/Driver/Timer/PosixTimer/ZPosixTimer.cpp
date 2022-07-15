#if !defined(_WIN32) && !defined(WIN32)
#include "ZPosixTimer.h"
#include <cassert>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

using namespace PaintsNow;

class PosixTimerImpl : public ITimer::Timer {
public:
	TWrapper<void, size_t> wrapper;
	size_t interval;
	timer_t timerID;
};

static void TimerFunc(sigval s) {
	PosixTimerImpl* q = reinterpret_cast<PosixTimerImpl*>(s.sival_ptr);
	if (q->interval == 0) {
		delete q;
	} else {
		q->wrapper(q->interval);
	}
}

ZPosixTimer::ZPosixTimer() {
}

ZPosixTimer::~ZPosixTimer() {
}

void ZPosixTimer::StopTimer(Timer* timer) {
	PosixTimerImpl* impl = static_cast<PosixTimerImpl*>(timer);
	impl->interval = 0;
	timer_delete(impl->timerID);
	// delete impl;
}

size_t ZPosixTimer::GetTimerInterval(Timer* timer) const {
	PosixTimerImpl* impl = static_cast<PosixTimerImpl*>(timer);
	return impl->interval;
}

ITimer::Timer* ZPosixTimer::StartTimer(size_t inter, const TWrapper<void, size_t>& w) {
	PosixTimerImpl* impl = new PosixTimerImpl();
	impl->wrapper = w;
	impl->interval = inter;

	sigevent sev;
	itimerspec its;
	// long long freq_nanosecs;
	// sigset_t mask;
	memset(&sev, 0, sizeof(sev));
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = TimerFunc;
	sev.sigev_value.sival_ptr = impl;
	timer_create(CLOCK_REALTIME, &sev, &impl->timerID);

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 1;
	its.it_interval.tv_sec = inter / 1000;
	its.it_interval.tv_nsec = (inter % 1000) * 1000000;

	timer_settime(impl->timerID, 0, &its, nullptr);
	return impl;
}

#endif // !_WIN32
