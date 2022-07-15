// ZThreadPthread.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once
#include "../../../Interface/IThread.h"

namespace PaintsNow {
	class ZThreadPthread : public IThread {
	public:
		ZThreadPthread();
		~ZThreadPthread() override;
		IThread::Thread* NewThread(const TWrapper<bool, IThread::Thread*, size_t>& wrapper, size_t index) override;
		bool IsThreadRunning(Thread* thread) const override;
		void Wait(Thread* thread) override;
		void DeleteThread(Thread* thread) override;

		IThread::Lock* NewLock() override;
		void DoLock(Lock* lock) override;
		void UnLock(Lock* lock) override;
		bool TryLock(Lock* lock) override;
		bool IsLocked(Lock* lock) override;
		void DeleteLock(Lock* lock) override;

		SharedLock* NewSharedLock() override;
		void DoLockWriter(SharedLock* lock) override;
		void DoLockReader(SharedLock* lock) override;
		void UnLockWriter(SharedLock* lock) override;
		void UnLockReader(SharedLock* lock) override;
		bool TryLockWriter(SharedLock* lock) override;
		bool TryLockReader(SharedLock* lock) override;
		bool IsLocked(SharedLock* lock) override;
		void DeleteLock(SharedLock* lock) override;

		Event* NewEvent() override;
		void Signal(Event* event) override;
		void Wait(Event* event, Lock* lock) override;
		void Wait(Event* event, Lock* lock, size_t timeout) override;
		void DeleteEvent(Event* event) override;
		void Sleep(size_t milliseconds) override;
	};
}

