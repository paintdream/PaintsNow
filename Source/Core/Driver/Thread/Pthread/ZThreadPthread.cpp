#include "ZThreadPthread.h"

#if defined(_DEBUG) && defined(_MSC_VER)
#include <Windows.h>
#endif

#if !defined(_MSC_VER) || _MSC_VER > 1200
#define USE_STD_THREAD
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#ifndef HAS_SHARED_MUTEX
#if __cplusplus > 201402L || (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
#define HAS_SHARED_MUTEX 1
#include <shared_mutex>
using shared_mutex = std::shared_mutex;
#elif __cplusplus == 201402L || (defined(_MSVC_LANG) && _MSVC_LANG == 201402L)
#define HAS_SHARED_MUTEX 1
#include <shared_mutex>
using shared_mutex = std::shared_timed_mutex;
#else
#define HAS_SHARED_MUTEX 0
#endif
#endif

#else
#include <windows.h>
#include <process.h>
#if _WIN32_WINNT >= 0x0600 // Windows Vista
#define HAS_CONDITION_VARIABLE 1
#define HAS_RWLOCK 1
#else
#define HAS_CONDITION_VARIABLE 0
#define HAS_RWLOCK 0
#endif
#endif

using namespace PaintsNow;

class LockImpl : public IThread::Lock {
public:
#ifdef USE_STD_THREAD
	std::mutex cs;
#else
	CRITICAL_SECTION cs;
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
	DWORD owner;
#endif
	size_t lockCount;
};

#define FLAG_WRITER 0x80000000

class SharedLockImpl : public IThread::SharedLock {
public:
#ifdef USE_STD_THREAD
	#if HAS_SHARED_MUTEX
	shared_mutex cs;
	#else
	std::condition_variable cv;
	std::mutex cs;
	std::atomic<uint32_t> flag;
	#endif
#else
	#if HAS_RWLOCK
		SRWLOCK cs;
	#else
		CRITICAL_SECTION cs;
		std::atomic<DWORD> flag;
	#endif
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
	DWORD owner;
#endif
	std::atomic<size_t> lockCount;
};

class ThreadImpl : public IThread::Thread {
public:
#ifdef USE_STD_THREAD
	std::thread thread;
#else
	HANDLE thread;
#endif

	TWrapper<bool, IThread::Thread*, size_t> proxy;
	size_t index;
	bool running;
	bool managed;
	bool reserved[2];
};

class EventImpl : public IThread::Event {
public:
#ifdef USE_STD_THREAD
	std::condition_variable cond;
#else
	#if HAS_CONDITION_VARIABLE
		CONDITION_VARIABLE cond;
	#else
		HANDLE cond;
	#endif
#endif
};

#ifdef USE_STD_THREAD
static void
#else
static UINT _stdcall
#endif
_ThreadProc(void* param)
{
	ThreadImpl* thread = reinterpret_cast<ThreadImpl*>(param);
	thread->running = true;
	bool deleteSelf = thread->proxy(thread, thread->index);

	if (deleteSelf) {
		thread->running = false;
		delete thread;
	}

#ifndef USE_STD_THREAD
	::_endthreadex(0);
	return 0;
#endif
}

ZThreadPthread::ZThreadPthread() {

}

ZThreadPthread::~ZThreadPthread() {
}

void ZThreadPthread::Wait(Thread* th) {
	ThreadImpl* t = static_cast<ThreadImpl*>(th);
#ifdef USE_STD_THREAD
	t->thread.join();
#else
	::WaitForSingleObject(t->thread, INFINITE);
#endif
	t->managed = false;
}

IThread::Thread* ZThreadPthread::NewThread(const TWrapper<bool, IThread::Thread*, size_t>& wrapper, size_t index) {
	ThreadImpl* t = new ThreadImpl();
	t->proxy = wrapper;
	t->index = index;
	t->managed = true;

#ifdef USE_STD_THREAD
	t->thread = std::thread(_ThreadProc, t);
#else
	t->thread = (HANDLE)::_beginthreadex(nullptr, 0, _ThreadProc, t, 0, nullptr);
#endif

	return t;
}

bool ZThreadPthread::IsThreadRunning(Thread* th) const {
	assert(th != nullptr);
	ThreadImpl* thread = static_cast<ThreadImpl*>(th);
	return thread->running;
}

void ZThreadPthread::DeleteThread(Thread* thread) {
	assert(thread != nullptr);
	ThreadImpl* t = static_cast<ThreadImpl*>(thread);
	if (t->managed) {
#ifdef USE_STD_THREAD
		t->thread.detach();
#else
		::CloseHandle(t->thread);
#endif
	}

	delete t;
}

IThread::Lock* ZThreadPthread::NewLock() {
	LockImpl* lock = new LockImpl();
	lock->lockCount = 0;
#if defined(_DEBUG) && defined(_MSC_VER)
	lock->owner = 0;
#endif

#ifndef USE_STD_THREAD
	::InitializeCriticalSection(&lock->cs);
#endif

	return lock;
}

void ZThreadPthread::DoLock(Lock* l) {
	assert(l != nullptr);
	LockImpl* lock = static_cast<LockImpl*>(l);
#if defined(_DEBUG) && defined(_MSC_VER)
	assert(lock->owner != ::GetCurrentThreadId());
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
	lock->owner = ::GetCurrentThreadId();
	// printf("Thread %d, takes: %p\n", lock->owner, lock);
#endif

#ifdef USE_STD_THREAD
	lock->cs.lock();
#else
	::EnterCriticalSection(&lock->cs);
#endif
	assert(lock->lockCount == 0);
	lock->lockCount++;
}

void ZThreadPthread::UnLock(Lock* l) {
	assert(l != nullptr);
	LockImpl* lock = static_cast<LockImpl*>(l);
	lock->lockCount--;

#if defined(_DEBUG) && defined(_MSC_VER)
	// printf("Thread %d, releases: %p\n", lock->owner, lock);
	lock->owner = ::GetCurrentThreadId();
	lock->owner = 0;
#endif

#ifdef USE_STD_THREAD
	lock->cs.unlock();
#else
	::LeaveCriticalSection(&lock->cs);
#endif
}

bool ZThreadPthread::TryLock(Lock* l) {
	assert(l != nullptr);
	LockImpl* lock = static_cast<LockImpl*>(l);
#ifdef USE_STD_THREAD
	bool success = lock->cs.try_lock();
#else
	bool success = ::TryEnterCriticalSection(&lock->cs) != 0;
#endif
	if (success) {
		lock->lockCount++;
	}

	return success;
}

void ZThreadPthread::DeleteLock(Lock* l) {
	assert(l != nullptr);
	LockImpl* lock = static_cast<LockImpl*>(l);
#ifndef USE_STD_THREAD
	::DeleteCriticalSection(&lock->cs);
#endif

	delete lock;
}

bool ZThreadPthread::IsLocked(Lock* l) {
	assert(l != nullptr);
	LockImpl* lock = static_cast<LockImpl*>(l);
	return lock->lockCount != 0;
}

IThread::Event* ZThreadPthread::NewEvent() {
	EventImpl* ev = new EventImpl();
#ifndef USE_STD_THREAD
#if HAS_CONDITION_VARIABLE
	::InitializeConditionVariable(&ev->cond);
#else
	ev->cond = ::CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
#endif
	return ev;
}

void ZThreadPthread::Signal(Event* event) {
	assert(event != nullptr);
	EventImpl* ev = static_cast<EventImpl*>(event);

#ifdef USE_STD_THREAD
	ev->cond.notify_one();
#else
#if HAS_CONDITION_VARIABLE
	::WakeConditionVariable(&ev->cond);
#else
	::SetEvent(ev->cond);
#endif
#endif
}

void ZThreadPthread::Wait(Event* event, Lock* lock, size_t timeout) {
	assert(event != nullptr);
	assert(lock != nullptr);
	EventImpl* ev = static_cast<EventImpl*>(event);
	LockImpl* l = static_cast<LockImpl*>(lock);
	assert(l->lockCount != 0);
	l->lockCount--; // it's safe because we still hold this lock

#ifdef USE_STD_THREAD
	std::unique_lock<std::mutex> u(l->cs, std::adopt_lock);
	ev->cond.wait_for(u, std::chrono::microseconds(timeout));
	u.release();
#else
#if HAS_CONDITION_VARIABLE
	::SleepConditionVariableCS(&ev->cond, &l->cs, (DWORD)timeout);
#else
	::LeaveCriticalSection(&l->cs);
	::WaitForSingleObject(ev->cond, (DWORD)timeout); // Windows Event's SetEvent can be called before WaitForSingleObject
	::EnterCriticalSection(&l->cs);
#endif
#endif
	l->lockCount++; // it's also safe because we has already take lock before returning from pthread_cond_wait
}

void ZThreadPthread::Wait(Event* event, Lock* lock) {
	assert(event != nullptr);
	assert(lock != nullptr);
	EventImpl* ev = static_cast<EventImpl*>(event);
	LockImpl* l = static_cast<LockImpl*>(lock);
	l->lockCount--; // it's safe because we still hold this lock

#if defined(_DEBUG) && defined(_MSC_VER)
	l->owner = 0;
#endif

#ifdef USE_STD_THREAD
	std::unique_lock<std::mutex> u(l->cs, std::adopt_lock);
	ev->cond.wait(u);
	u.release();
#else
#if HAS_CONDITION_VARIABLE
	::SleepConditionVariableCS(&ev->cond, &l->cs, INFINITE);
#else
	::LeaveCriticalSection(&l->cs);
	::WaitForSingleObject(ev->cond, INFINITE); // It's OK to unlock and wait here. Because windows event object works even `signal` happened before `wait`.
	::EnterCriticalSection(&l->cs);
#endif
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
	l->owner = ::GetCurrentThreadId();
#endif

	l->lockCount++; // it's also safe because we has already take lock before returning from pthread_cond_wait
}

void ZThreadPthread::DeleteEvent(Event* event) {
	assert(event != nullptr);
	EventImpl* ev = static_cast<EventImpl*>(event);
#ifndef USE_STD_THREAD
#if !HAS_CONDITION_VARIABLE
	::CloseHandle(ev->cond);
#endif
#endif

	delete ev;
}

void ZThreadPthread::Sleep(size_t milliseconds) {
#ifdef USE_STD_THREAD
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#else
	::Sleep((DWORD)milliseconds);
#endif
}

IThread::SharedLock* ZThreadPthread::NewSharedLock() {
	SharedLockImpl* lock = new SharedLockImpl();
	lock->lockCount.store(0, std::memory_order_relaxed);
#if defined(_DEBUG) && defined(_MSC_VER)
	lock->owner = 0;
#endif

#ifdef USE_STD_THREAD
#if !HAS_SHARED_MUTEX
	lock->flag.store(0, std::memory_order_relaxed);
#endif
#else
#if HAS_RWLOCK
	::InitializeSRWLock(&lock->cs);
#else
	::InitializeCriticalSection(&lock->cs);
	lock->flag.store(0, std::memory_order_relaxed);
#endif
#endif

	return lock;
}

void ZThreadPthread::DoLockWriter(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
#if defined(_DEBUG) && defined(_MSC_VER)
	assert(lock->owner != ::GetCurrentThreadId());
	lock->owner = ::GetCurrentThreadId();
	// printf("Thread %d, takes: %p\n", lock->owner, lock);
#endif

#ifdef USE_STD_THREAD
#if HAS_SHARED_MUTEX
	lock->cs.lock();
#else
	uint32_t expected = 0;
	if (lock->flag.compare_exchange_strong(expected, FLAG_WRITER, std::memory_order_acquire)) {
		lock->cs.lock();
	} else {
		while (true) {
			std::unique_lock<std::mutex> guard(lock->cs);
			lock->cv.wait(guard);

			uint32_t expected = 0;
			if (lock->flag.compare_exchange_weak(expected, FLAG_WRITER, std::memory_order_acquire)) {
				guard.release();
				break;
			}
		}
	}
#endif
#else
#if HAS_RWLOCK
	::AcquireSRWLockExclusive(&lock->cs);
#else
	DWORD expected = 0;
	if (lock->flag.compare_exchange_strong(expected, FLAG_WRITER, std::memory_order_acquire)) {
		::EnterCriticalSection(&lock->cs);
	} else {
		while (true) {
			::EnterCriticalSection(&lock->cs);
			DWORD expected = 0;
			if (lock->flag.compare_exchange_weak(expected, FLAG_WRITER, std::memory_order_acquire)) {
				break;
			} else {
				::LeaveCriticalSection(&lock->cs);
			}
		}
	}
#endif
#endif
	assert(lock->lockCount.load(std::memory_order_acquire) == 0);
	lock->lockCount.fetch_add(1, std::memory_order_relaxed);
}

void ZThreadPthread::DoLockReader(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
#if defined(_DEBUG) && defined(_MSC_VER)
	// assert(lock->owner != ::GetCurrentThreadId());
	// lock->owner = ::GetCurrentThreadId();
	// printf("Thread %d, takes: %p\n", lock->owner, lock);
#endif

#ifdef USE_STD_THREAD
#if HAS_SHARED_MUTEX
	lock->cs.lock_shared();
#else
	uint32_t value = lock->flag.load(std::memory_order_acquire);
	while (true) {
		if (!(value & FLAG_WRITER)) {
			if (lock->flag.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) {
				break; // read lock acquired
			} // retry
		} else {
			// locked by writer?
			std::lock_guard<std::mutex> guard(lock->cs);
			value = lock->flag.load(std::memory_order_acquire);
		}
	}
#endif
#else
#if HAS_RWLOCK
	::AcquireSRWLockShared(&lock->cs);
#else
	DWORD value = lock->flag.load(std::memory_order_acquire);
	while (true) {
		if (!(value & FLAG_WRITER)) {
			if (lock->flag.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) {
				if (value == 0) {
					::EnterCriticalSection(&lock->cs);
				}

				break; // read lock acquired
			} // retry
		} else {
			// locked by writer?
			::EnterCriticalSection(&lock->cs);
			value = lock->flag.load(std::memory_order_acquire);
			::LeaveCriticalSection(&lock->cs);
		}
	}
#endif
#endif
	lock->lockCount.fetch_add(1, std::memory_order_relaxed);
}

void ZThreadPthread::UnLockWriter(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
	lock->lockCount.fetch_sub(1, std::memory_order_relaxed);

#if defined(_DEBUG) && defined(_MSC_VER)
	// printf("Thread %d, releases: %p\n", lock->owner, lock);
	lock->owner = ::GetCurrentThreadId();
	lock->owner = 0;
#endif

#ifdef USE_STD_THREAD
#if HAS_SHARED_MUTEX
	lock->cs.unlock();
#else
	assert(lock->flag.load(std::memory_order_acquire) & FLAG_WRITER);
	lock->flag.store(0, std::memory_order_release);
	lock->cv.notify_one();
	lock->cs.unlock();
#endif
#else
#if HAS_RWLOCK
	::ReleaseSRWLockExclusive(&lock->cs);
#else
	assert(lock->flag.load(std::memory_order_acquire) & FLAG_WRITER);
	lock->flag.store(0, std::memory_order_release);
	::LeaveCriticalSection(&lock->cs);
#endif
#endif
}

void ZThreadPthread::UnLockReader(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
	lock->lockCount.fetch_sub(1, std::memory_order_relaxed);
	assert(lock->lockCount.load(std::memory_order_relaxed) < 0x80000000);
#if defined(_DEBUG) && defined(_MSC_VER)
	// printf("Thread %d, releases: %p\n", lock->owner, lock);
	// lock->owner = ::GetCurrentThreadId();
	// lock->owner = 0;
#endif

#ifdef USE_STD_THREAD
#if HAS_SHARED_MUTEX
	lock->cs.unlock_shared();
#else
	assert(!(lock->flag.load(std::memory_order_acquire) & FLAG_WRITER));
	if (lock->flag.fetch_sub(1, std::memory_order_relaxed) == 1) { // last reader
		lock->cv.notify_one();
	}
#endif
#else
#if HAS_RWLOCK
	::ReleaseSRWLockShared(&lock->cs);
#else
	assert(!(lock->flag.load(std::memory_order_acquire) & FLAG_WRITER));
	if (lock->flag.fetch_sub(1, std::memory_order_relaxed) == 1) { // last reader
		::LeaveCriticalSection(&lock->cs);
	}
#endif
#endif
}

bool ZThreadPthread::TryLockWriter(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
#ifdef USE_STD_THREAD
#if HAS_SHARED_MUTEX
	bool success = lock->cs.try_lock();
#else
	uint32_t expected = 0;
	bool success = lock->flag.compare_exchange_strong(expected, FLAG_WRITER, std::memory_order_acquire);
	if (success) {
		lock->cs.lock();
	}
#endif
#else
#if HAS_RWLOCK
	bool success = ::TryAcquireSRWLockExclusive(&lock->cs) != 0;
#else
	DWORD expected = 0;
	bool success = lock->flag.compare_exchange_strong(expected, FLAG_WRITER, std::memory_order_acquire);
	if (success) {
		::EnterCriticalSection(&lock->cs);
	}
#endif
#endif
	if (success) {
		lock->lockCount.fetch_add(1, std::memory_order_relaxed);
	}

	return success;
}

bool ZThreadPthread::TryLockReader(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
#ifdef USE_STD_THREAD
#if HAS_SHARED_MUTEX
	bool success = lock->cs.try_lock_shared();
#else
	uint32_t value = lock->flag.load(std::memory_order_acquire);
	bool success = false;
	while (!(value & FLAG_WRITER)) {
		if (lock->flag.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) {
			success = true;
			break;
		}
	}
#endif
#else
#if HAS_RWLOCK
	bool success = ::TryAcquireSRWLockShared(&lock->cs) != 0;
#else
	DWORD value = lock->flag.load(std::memory_order_acquire);
	bool success = false;
	while (!(value & FLAG_WRITER)) {
		if (lock->flag.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) {
			success = true;

			if (value == 0) {
				::EnterCriticalSection(&lock->cs);
			}
		}
	}
#endif
#endif
	if (success) {
		lock->lockCount.fetch_add(1, std::memory_order_relaxed);
	}

	return success;
}

bool ZThreadPthread::IsLocked(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
	return lock->lockCount.load(std::memory_order_acquire) != 0;
}

void ZThreadPthread::DeleteLock(SharedLock* l) {
	assert(l != nullptr);
	SharedLockImpl* lock = static_cast<SharedLockImpl*>(l);
#ifndef USE_STD_THREAD
#if !HAS_RWLOCK
	::DeleteCriticalSection(&lock->cs);
#endif
#endif

	delete lock;
}
