#include "ITimer.h"

#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif

using namespace PaintsNow;

ITimer::~ITimer() {}

int64_t ITimer::GetSystemClock() {
#ifdef _WIN32
	LARGE_INTEGER TicksPerSecond = { 0 };
	LARGE_INTEGER Tick;
	QueryPerformanceFrequency(&TicksPerSecond);
	QueryPerformanceCounter(&Tick);
	uint64_t Seconds = Tick.QuadPart / TicksPerSecond.QuadPart;
	uint64_t LeftPart = Tick.QuadPart - (TicksPerSecond.QuadPart*Seconds);
	uint64_t MillSeconds = LeftPart * 1000 / TicksPerSecond.QuadPart;
	return Seconds * 1000 + MillSeconds;
#else
	struct timeval	tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}
