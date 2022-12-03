// IThread.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once

#include "../PaintsNow.h"
#include "IType.h"
#include "IDevice.h"
#include "../Template/TProxy.h"
#include "../Template/TAtomic.h"

namespace PaintsNow {
	class pure_interface IThread : public IDevice {
	public:
		~IThread() override;
		class Lock {};
		class SharedLock {};
		class Semaphore {};
		class Thread {};
		class Event {};
	
		static uint32_t GetCurrentNativeThreadId();
		virtual Thread* NewThread(const TWrapper<bool, Thread*, size_t>& wrapper, size_t index) = 0;
		virtual void SetThreadName(Thread* thread, const String& name) = 0;
		virtual bool IsThreadRunning(Thread* thread) const = 0;
		virtual void Wait(Thread* thread) = 0;
		virtual void DeleteThread(Thread* thread) = 0;

		virtual Lock* NewLock() = 0;
		virtual void DoLock(Lock* lock) = 0;
		virtual void UnLock(Lock* lock) = 0;
		virtual bool TryLock(Lock* lock) = 0;
		virtual bool IsLocked(Lock* lock) = 0;
		virtual void DeleteLock(Lock* lock) = 0;

		virtual SharedLock* NewSharedLock() = 0;
		virtual void DoLockWriter(SharedLock* lock) = 0;
		virtual void DoLockReader(SharedLock* lock) = 0;
		virtual void UnLockWriter(SharedLock* lock) = 0;
		virtual void UnLockReader(SharedLock* lock) = 0;
		virtual bool TryLockWriter(SharedLock* lock) = 0;
		virtual bool TryLockReader(SharedLock* lock) = 0;
		virtual bool IsLocked(SharedLock* lock) = 0;
		virtual void DeleteLock(SharedLock* lock) = 0;

		virtual Event* NewEvent() = 0;
		virtual void Signal(Event* event) = 0;
		virtual void Wait(Event* event, Lock* lock) = 0;
		virtual void Wait(Event* event, Lock* lock, size_t timeout) = 0;
		virtual void DeleteEvent(Event* event) = 0;
		virtual void Sleep(size_t milliseconds) = 0;
	};

	class LockGuard {
	public:
		LockGuard(IThread& thread, IThread::Lock* mutex);
		~LockGuard();
	private:
		IThread& thread;
		IThread::Lock* mutex;
	};

	class SharedLockGuardReader {
	public:
		SharedLockGuardReader(IThread& thread, IThread::SharedLock* mutex);
		~SharedLockGuardReader();

		void UnLock();
	private:
		IThread& thread;
		IThread::SharedLock* mutex;
	};

	class SharedLockGuardWriter {
	public:
		SharedLockGuardWriter(IThread& thread, IThread::SharedLock* mutex);
		~SharedLockGuardWriter();

		void UnLock();
	private:
		IThread& thread;
		IThread::SharedLock* mutex;
	};

	class ISyncObject {
	public:
		ISyncObject(IThread& threadApi);
		virtual ~ISyncObject();
		void DoLock();
		void UnLock();

		bool TryLock();
		bool IsLocked() const;
		IThread& GetThreadApi();
		IThread::Lock* GetLock() const;

	protected:
		IThread& threadApi;
		IThread::Lock* mutex;
	};

	class ISyncObjectShared {
	public:
		ISyncObjectShared(IThread& threadApi);
		virtual ~ISyncObjectShared();
		void DoLockWriter();
		void UnLockWriter();
		bool TryLockWriter();
		void DoLockReader();
		void UnLockReader();
		bool TryLockReader();
		bool IsLocked() const;
		IThread& GetThreadApi();
		IThread::SharedLock* GetLock() const;

	protected:
		IThread& threadApi;
		IThread::SharedLock* mutex;
	};
}

