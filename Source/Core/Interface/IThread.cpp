#include "IThread.h"
#ifdef _WIN32
#include <windows.h>
#endif

using namespace PaintsNow;

IThread::~IThread() {}

uint32_t IThread::GetCurrentNativeThreadId() {
#ifdef _WIN32
	return ::GetCurrentThreadId();
#else
	return 0;
#endif	
}

LockGuard::LockGuard(IThread& t, IThread::Lock* l) : thread(t), mutex(l) {
	thread.DoLock(mutex);
}

LockGuard::~LockGuard() {
	thread.UnLock(mutex);
}

SharedLockGuardReader::SharedLockGuardReader(IThread& t, IThread::SharedLock* l) : thread(t), mutex(l) {
	thread.DoLockReader(mutex);
}

SharedLockGuardReader::~SharedLockGuardReader() {
	if (mutex != nullptr) {
		thread.UnLockReader(mutex);
	}
}

void SharedLockGuardReader::UnLock() {
	assert(mutex != nullptr);
	thread.UnLockReader(mutex);
	mutex = nullptr;
}

SharedLockGuardWriter::SharedLockGuardWriter(IThread& t, IThread::SharedLock* l) : thread(t), mutex(l) {
	thread.DoLockWriter(mutex);
}

SharedLockGuardWriter::~SharedLockGuardWriter() {
	if (mutex != nullptr) {
		thread.UnLockWriter(mutex);
	}
}

void SharedLockGuardWriter::UnLock() {
	assert(mutex != nullptr);
	thread.UnLockWriter(mutex);
	mutex = nullptr;
}


ISyncObject::~ISyncObject() {
	threadApi.DeleteLock(mutex);
}

ISyncObject::ISyncObject(IThread& thread) : threadApi(thread) {
	mutex = threadApi.NewLock();
}

void ISyncObject::DoLock() {
	threadApi.DoLock(mutex);
}

void ISyncObject::UnLock() {
	threadApi.UnLock(mutex);
}

bool ISyncObject::TryLock() {
	return threadApi.TryLock(mutex);
}

bool ISyncObject::IsLocked() const {
	return threadApi.IsLocked(mutex);
}

IThread& ISyncObject::GetThreadApi() {
	return threadApi;
}

IThread::Lock* ISyncObject::GetLock() const {
	return mutex;
}

ISyncObjectShared::~ISyncObjectShared() {
	threadApi.DeleteLock(mutex);
}

ISyncObjectShared::ISyncObjectShared(IThread& thread) : threadApi(thread) {
	mutex = threadApi.NewSharedLock();
}

void ISyncObjectShared::DoLockWriter() {
	threadApi.DoLockWriter(mutex);
}

void ISyncObjectShared::UnLockWriter() {
	threadApi.UnLockWriter(mutex);
}

bool ISyncObjectShared::TryLockWriter() {
	return threadApi.TryLockWriter(mutex);
}

void ISyncObjectShared::DoLockReader() {
	threadApi.DoLockReader(mutex);
}

void ISyncObjectShared::UnLockReader() {
	threadApi.UnLockReader(mutex);
}

bool ISyncObjectShared::TryLockReader() {
	return threadApi.TryLockReader(mutex);
}

bool ISyncObjectShared::IsLocked() const {
	return threadApi.IsLocked(mutex);
}

IThread& ISyncObjectShared::GetThreadApi() {
	return threadApi;
}

IThread::SharedLock* ISyncObjectShared::GetLock() const {
	return mutex;
}
