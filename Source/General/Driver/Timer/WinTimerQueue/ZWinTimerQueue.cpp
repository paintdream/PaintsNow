#if defined(_WIN32) || defined(WIN32)
#include "ZWinTimerQueue.h"
#include <cassert>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>

using namespace PaintsNow;

class TimerController {
public:
	CRITICAL_SECTION cs;
	TWrapper<void, size_t> wrapper;
	size_t interval;
	HANDLE handle;
	HANDLE timer;
	bool deleteMark;
};

class WinTimerQueueImpl : public ITimer::Timer {
public:
	HANDLE timerQueue;
	TWrapper<void, size_t> wrapper;
	TimerController* controller;
	size_t interval;
};

static VOID WINAPI TimerFunc(PVOID pContext, BOOLEAN bTimeOrWait) {
	TimerController* q = reinterpret_cast<TimerController*>(pContext);
	if (::TryEnterCriticalSection(&q->cs)) { // Do not alert it at busy
		q->wrapper(q->interval);
		::LeaveCriticalSection(&q->cs);
	}
}

ZWinTimerQueue::ZWinTimerQueue() {}

ZWinTimerQueue::~ZWinTimerQueue() {}

static DWORD WINAPI DeleteTimerFunc(LPVOID pContext) {
	TimerController* q = reinterpret_cast<TimerController*>(pContext);
	::DeleteTimerQueueTimer(q->handle, q->timer, INVALID_HANDLE_VALUE);
	::EnterCriticalSection(&q->cs);
	::DeleteTimerQueue(q->handle);
	::LeaveCriticalSection(&q->cs);
	::DeleteCriticalSection(&q->cs);

	delete q;
	return 0;
}

void ZWinTimerQueue::StopTimer(Timer* timer) {
	WinTimerQueueImpl* impl = static_cast<WinTimerQueueImpl*>(timer);

	if (impl->controller != nullptr) {
		// ::QueueUserWorkItem(DeleteTimerFunc, controller, 0);
		::DeleteTimerFunc(impl->controller);
		impl->controller = nullptr;
		delete impl;
	}
}

size_t ZWinTimerQueue::GetTimerInterval(Timer* timer) const {
	WinTimerQueueImpl* impl = static_cast<WinTimerQueueImpl*>(timer);
	return impl->interval;
}

ITimer::Timer* ZWinTimerQueue::StartTimer(size_t inter, const TWrapper<void, size_t>& w) {
	WinTimerQueueImpl* impl = new WinTimerQueueImpl();

	impl->wrapper = w;
	impl->interval = inter;
	impl->controller = new TimerController();
	::InitializeCriticalSection(&impl->controller->cs);
	impl->controller->wrapper = impl->wrapper;
	impl->controller->interval = impl->interval;
	impl->timerQueue = ::CreateTimerQueue();
	impl->controller->handle = impl->timerQueue;
	impl->controller->timer = nullptr;
	::CreateTimerQueueTimer(&impl->controller->timer, impl->timerQueue, TimerFunc, impl->controller, 0, (DWORD)impl->interval, 0);

	return impl;
}

#endif // _WIN32