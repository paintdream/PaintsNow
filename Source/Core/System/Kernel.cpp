#include "Kernel.h"
#include "../Driver/Profiler/Optick/optick.h"
using namespace PaintsNow;

enum {
	STATE_IDLE = 0u, // task (list) is empty
	STATE_QUEUEING = 1u, // task (list) is scheduled to be flushed
	STATE_FLUSHING = 2u // task (list) is being flushed
};

void WarpTiny::SetWarpIndex(uint32_t warpIndex) {
	FLAG old, value;
	
	// Update warp index atomically
	do {
		old = value = flag.load(std::memory_order_acquire);
		value = (value & ~(WARP_INDEX_END - WARP_INDEX_BEGIN)) | (warpIndex << Math::Log2x((uint32_t)WARP_INDEX_BEGIN));
	} while (!flag.compare_exchange_weak(old, value, std::memory_order_release, std::memory_order_relaxed));
}

uint32_t WarpTiny::GetWarpIndex() const {
	return (flag.load(std::memory_order_relaxed) & (WARP_INDEX_END - WARP_INDEX_BEGIN)) >> Math::Log2x((uint32_t)WARP_INDEX_BEGIN);
}

void WarpTiny::AssertWarp(Kernel& kernel) const {
	assert(GetWarpIndex() == kernel.GetCurrentWarpIndex());
}

bool WarpTiny::WaitWarp(Kernel& kernel) {
	return kernel.WaitWarp(GetWarpIndex());
}

Kernel::Kernel(ThreadPool& tp, uint32_t warpCount) : threadPool(tp), maxParallelCount(0) {
	uint32_t threadCount = (uint32_t)threadPool.GetThreadCount();

	assert(warpCount <= (1u << WarpTiny::WARP_BITS));
	memset(fullFlushMask, 0, sizeof(fullFlushMask));
	memset(idleMask, 0, sizeof(idleMask));

	for (uint32_t i = 0; i < warpCount; i++) {
		idleMask[i / (8 * sizeof(size_t))] |= (size_t)1 << (i % (8 * sizeof(size_t)));
	}

	SetMaxParallelCount(0);
	anyFlush.store(STATE_IDLE, std::memory_order_relaxed);
	resetting.store(0, std::memory_order_relaxed);
	currentParallelCount.store(0, std::memory_order_release);
	// but not exceeds the warp bits restriction.
	taskQueueGrid.resize(warpCount, SubTaskQueue(this, verify_cast<uint32_t>(threadCount)));

#ifdef _DEBUG
	activeTaskCount.store(0, std::memory_order_relaxed);
#endif
}

Kernel::~Kernel() { Reset(); }

uint32_t Kernel::GetWarpCount() const {
	return verify_cast<uint32_t>(taskQueueGrid.size());
}

void Kernel::SetMaxParallelCount(uint32_t count) {
	maxParallelCount = count == 0 ? threadPool.GetThreadCount() : count;
}

void Kernel::SetWarpPriority(uint32_t warp, int priority) {
	assert(warp < taskQueueGrid.size());
	taskQueueGrid[warp].SetPriority(priority);
}

bool Kernel::AcquireFlush() {
	uint32_t count = currentParallelCount.load(std::memory_order_acquire);
	while (count < maxParallelCount) {
		if (currentParallelCount.compare_exchange_weak(count, count + 1, std::memory_order_relaxed)) {
			return true;
		}
	}

	return false;
}

void Kernel::ReleaseFlush() {
	uint32_t count = currentParallelCount.fetch_sub(1, std::memory_order_relaxed);
	assert(count != 0);

	uint32_t expected;
	do {
		if (anyFlush.exchange(STATE_FLUSHING, std::memory_order_acquire) != STATE_IDLE) {
			for (size_t i = 0; i < sizeof(fullFlushMask) / sizeof(fullFlushMask[0]); i++) {
				std::atomic<size_t>& s = *reinterpret_cast<std::atomic<size_t>*>(&fullFlushMask[i]);
				size_t mask = s.load(std::memory_order_acquire);
				while (mask != 0) {
					size_t bit = Math::Alignment(mask);
					if ((mask = s.fetch_and(~bit, std::memory_order_relaxed)) & bit) {
						size_t k = Math::Log2x(bit) + i * sizeof(size_t) * 8;
						SubTaskQueue& q = taskQueueGrid[k];
						if (q.queueing.exchange(STATE_IDLE, std::memory_order_relaxed) == STATE_QUEUEING) {
							if (q.Flush(threadPool)) {
								return;
							}
						}
					}
				}
			}
		}

		expected = STATE_FLUSHING;
	} while (!anyFlush.compare_exchange_weak(expected, STATE_IDLE, std::memory_order_release));
}

ThreadPool& Kernel::GetThreadPool() {
	return threadPool;
}

// Warp index is thread_local
thread_local uint32_t WarpIndex = ~(uint32_t)0;

uint32_t Kernel::GetCurrentWarpIndex() const {
	return WarpIndex;
}

#ifdef _DEBUG
thread_local bool InWarpRoutine = false;
#endif
	
bool Kernel::WaitWarpInternal(uint32_t warpIndex, uint32_t delay) {
	assert(warpIndex != ~(uint32_t)0);
	uint32_t threadIndex = threadPool.GetCurrentThreadIndex();
	SubTaskQueue& queue = taskQueueGrid[warpIndex];
	while (!queue.PreemptExecution() && threadPool.IsRunning()) {
		threadPool.PollDelay(threadIndex, delay);
	}

	return threadPool.IsRunning();
}

void Kernel::AcquireFlushOnWait() {
	uint32_t count = currentParallelCount.load(std::memory_order_acquire);
	uint32_t threadIndex = threadPool.GetCurrentThreadIndex();
	while (count < maxParallelCount) {
		if (currentParallelCount.compare_exchange_weak(count, count + 1, std::memory_order_relaxed)) {
			break;
		} else {
			threadPool.PollDelay(threadIndex, 5);
		}
	}
}

#ifdef _DEBUG
void Kernel::CheckSelfPolling(uint32_t warpIndex) {
	assert(warpIndex < taskQueueGrid.size());
	uint32_t current = WarpIndex;
	assert(taskQueueGrid[warpIndex].selfPolling.load(std::memory_order_acquire) == 0);
}
#endif

bool Kernel::Wait(std::atomic<uint32_t>& variable, uint32_t mask, uint32_t flag, uint32_t delay) {
	if ((variable.load(std::memory_order_acquire) & mask) == flag) {
		return true;
	}

	if (!threadPool.IsRunning())
		return false;

	OPTICK_EVENT();
	uint32_t current = WarpIndex;
	if (current != ~(uint32_t)0) {
#ifdef _DEBUG
		assert(InWarpRoutine);
#endif
		YieldCurrentWarp();
		ReleaseFlush();
		threadPool.PollWait(threadPool.GetCurrentThreadIndex(), variable, mask, flag, delay);
		bool ret = WaitWarpInternal(current, delay);
		AcquireFlushOnWait();
		return ret;
	} else {
		return threadPool.PollWait(threadPool.GetCurrentThreadIndex(), variable, mask, flag, delay);
	}
}

bool Kernel::WaitWarp(uint32_t warpIndex, uint32_t delay) {
	assert(warpIndex < taskQueueGrid.size());
	uint32_t current = WarpIndex;
	if (current == warpIndex)
		return true;

	if (!threadPool.IsRunning())
		return false;

	OPTICK_EVENT();
	if (warpIndex != ~(uint32_t)0) {
		if (current != ~(uint32_t)0) {
#ifdef _DEBUG
		assert(InWarpRoutine);
#endif
			YieldCurrentWarp();
			ReleaseFlush();
			bool ret = WaitWarpInternal(warpIndex, delay);
			AcquireFlushOnWait();
			return ret;
		} else {
			return WaitWarpInternal(warpIndex, delay);
		}
	} else {
		if (current != ~(uint32_t)0) {
			YieldCurrentWarp();
		}

		return threadPool.IsRunning();
	}
}

void Kernel::Reset() {
	OPTICK_EVENT();
	assert(GetCurrentWarpIndex() == ~(uint32_t)0);
	// Suspend all warps so we can take over tasks
	uint32_t warpCount = GetWarpCount();
	for (uint32_t i = 0; i < warpCount; i++) {
		SuspendWarp(i);
	}

	// Just abort them
	uint32_t threadIndex = threadPool.GetCurrentThreadIndex();
	threadIndex = threadIndex == ~(uint32_t)0 ? 0 : threadIndex;

	resetting.store(1, std::memory_order_release);
	do {
		if (threadPool.GetThreadCount() != 0) {
			std::vector<uint32_t> visited(GetWarpCount(), 0);

			while (true) {
				bool repeat = false;
				for (uint32_t j = 0; j < GetWarpCount(); j++) {
					if (visited[j] == 0) {
						SubTaskQueue& q = taskQueueGrid[j];
						SubTaskQueue::PreemptGuard<true> guard(q, j);
						if (guard) {
							q.AbortImpl(nullptr);
							visited[j] = 1;
						} else {
							repeat = true;
						}
					}
				}

				if (!repeat)
					break;

				if (!threadPool.Poll(threadIndex)) {
					YieldThread();
				}
			}
		} else {
			for (uint32_t j = 0; j < warpCount; j++) {
				SubTaskQueue& q = taskQueueGrid[j];
				SubTaskQueue::PreemptGuard<true> guard(q, j);
				assert(guard);
				q.AbortImpl(nullptr);
			}
		}
	} while (currentParallelCount.load(std::memory_order_acquire) != 0);

	for (uint32_t j = 0; j < warpCount; j++) {
		SubTaskQueue& q = taskQueueGrid[j];
		q.queueing.store(STATE_IDLE, std::memory_order_release);
	}

	resetting.store(0, std::memory_order_release);

	// Resume warps
	for (uint32_t k = 0; k < warpCount; k++) {
		ResumeWarp(k);
	}

	assert(currentParallelCount.load(std::memory_order_acquire) == 0);

#ifdef _DEBUG
	// assert(activeTaskCount.load(std::memory_order_acquire) == 0);
	// taskQueueGrid.clear();
#endif
}

Kernel::ForwardRoutine::ForwardRoutine(Kernel& k, WarpTiny* tn, ITask* tk) : kernel(k), tiny(tn), task(tk) {}

void Kernel::ForwardRoutine::Execute(void* context) {
	OPTICK_EVENT();
	assert(next == nullptr);
	assert(queued == 0);
	// requeue it
	kernel.QueueRoutinePost(tiny, task);
	tiny->ReleaseObject();

	ITask::Delete(this);
}

void Kernel::ForwardRoutine::Abort(void* context) {
	OPTICK_EVENT();
	assert(next == nullptr);
	assert(queued == 0);
	// force
	WarpIndex = tiny->GetWarpIndex();
	task->Abort(context);
	tiny->ReleaseObject();

	ITask::Delete(this);
}

inline void Kernel::QueueRoutineInternal(uint32_t toWarpIndex, uint32_t fromThreadIndex, WarpTiny* warpTiny, ITask* task) {
	// Different warp-thread pair indicates different queue grid, so it's thread safe.
	SubTaskQueue& queue = taskQueueGrid[toWarpIndex];
	queue.Push(fromThreadIndex, task, warpTiny);
	queue.Flush(threadPool);
}

// QueueRoutine from any thread in threadPool
void Kernel::QueueRoutine(WarpTiny* warpTiny, ITask* task) {
	assert(warpTiny != nullptr);
	uint32_t fromThreadIndex = threadPool.GetCurrentThreadIndex();
	uint32_t toWarpIndex = warpTiny->GetWarpIndex();
	assert(toWarpIndex < taskQueueGrid.size());
	SubTaskQueue& q = taskQueueGrid[toWarpIndex];
	if (fromThreadIndex == ~(uint32_t)0) {
		QueueRoutineExternal(warpTiny, task);
	} else {
		SubTaskQueue::PreemptGuard<false> guard(q, toWarpIndex);
		if (guard) {
			// Just the same warp? Execute at once.
#ifdef _DEBUG
			q.selfPolling.fetch_add(1, std::memory_order_acquire);
#endif
			task->Execute(threadPool.GetThreadContext(fromThreadIndex));
#ifdef _DEBUG
			q.selfPolling.fetch_sub(1, std::memory_order_release);
#endif
		} else {
			QueueRoutinePost(warpTiny, task);
		}
	}
}

void Kernel::QueueRoutineExternal(WarpTiny* warpTiny, ITask* task) {
	assert(warpTiny != nullptr);
	uint32_t fromThreadIndex = threadPool.GetCurrentThreadIndex();
	uint32_t toWarpIndex = warpTiny->GetWarpIndex();
	assert(toWarpIndex < taskQueueGrid.size());
	SubTaskQueue& q = taskQueueGrid[toWarpIndex];
	assert(fromThreadIndex == ~(uint32_t)0);
	// external thread?
	// forward to threadPool directly
	warpTiny->ReferenceObject();
	threadPool.Dispatch(new (ITask::Allocate(sizeof(ForwardRoutine))) ForwardRoutine(*this, warpTiny, task));
}

void Kernel::QueueRoutinePost(WarpTiny* warpTiny, ITask* task) {
	assert(warpTiny != nullptr);
	uint32_t fromThreadIndex = threadPool.GetCurrentThreadIndex();
	uint32_t toWarpIndex = warpTiny->GetWarpIndex();
	assert(toWarpIndex < taskQueueGrid.size());
#ifdef _DEBUG
	activeTaskCount.fetch_add(1, std::memory_order_relaxed);
#endif
	warpTiny->ReferenceObject(); // hold reference in case of invalid memory access.
	QueueRoutineInternal(toWarpIndex, fromThreadIndex, warpTiny, task);
}

uint32_t Kernel::YieldCurrentWarp() {
	uint32_t warpIndex = WarpIndex;
	if (warpIndex != ~(uint32_t)0) {
		taskQueueGrid[warpIndex].YieldExecution();
	}

	return warpIndex;
}

bool Kernel::SuspendWarp(uint32_t warpIndex) {
	assert(warpIndex < taskQueueGrid.size());
	SubTaskQueue& taskQueue = taskQueueGrid[warpIndex];
	return taskQueue.Suspend();
}

bool Kernel::ResumeWarp(uint32_t warpIndex) {
	assert(warpIndex < taskQueueGrid.size());
	SubTaskQueue& taskQueue = taskQueueGrid[warpIndex];
	return taskQueue.Resume();
}

// SubTaskQueue
Kernel::SubTaskQueue::SubTaskQueue(Kernel* e, uint32_t idCount) : kernel(e), TaskQueue(idCount), priority(0), stackQueue(~(uint32_t)0) {
	threadWarp.store(nullptr, std::memory_order_relaxed);
	suspendCount.store(0, std::memory_order_relaxed);
#ifdef _DEBUG
	selfPolling.store(0, std::memory_order_relaxed);
#endif
	queueing.store(0, std::memory_order_release);

	YieldExecution();
}

void Kernel::SubTaskQueue::SetPriority(int i) {
	priority = i;
}

Kernel::SubTaskQueue::SubTaskQueue(const SubTaskQueue& rhs) : TaskQueue(verify_cast<uint32_t>(rhs.ringBuffers.size())) {
	kernel = rhs.kernel;
	priority = rhs.priority;
	stackQueue = rhs.stackQueue;
	threadWarp.store(nullptr, std::memory_order_relaxed);
	suspendCount.store(0, std::memory_order_relaxed);
#ifdef _DEBUG
	selfPolling.store(0, std::memory_order_relaxed);
#endif
	queueing.store(0, std::memory_order_release);

	YieldExecution();
}

inline bool Kernel::SubTaskQueue::Suspend() {
	return suspendCount.fetch_add(1, std::memory_order_acquire) == 0;
}

inline bool Kernel::SubTaskQueue::Resume() {
	bool ret = suspendCount.fetch_sub(1, std::memory_order_release) == 1;

	if (ret) {
		if (queueing.exchange(STATE_IDLE, std::memory_order_relaxed) == STATE_QUEUEING) {
			Flush(kernel->threadPool);
		}
	}

	return ret;
}

Kernel::SubTaskQueue& Kernel::SubTaskQueue::operator = (const SubTaskQueue& rhs) {
	ringBuffers.resize(rhs.ringBuffers.size());
	kernel = rhs.kernel;

	return *this;
}

Kernel::SubTaskQueue::~SubTaskQueue() {}

bool Kernel::SubTaskQueue::Flush(ThreadPool& threadPool) {
	if (kernel->resetting.load(std::memory_order_acquire) != 0)
		return true;

	// avoid duplicated flushes
	if (queueing.exchange(STATE_QUEUEING, std::memory_order_relaxed) == STATE_IDLE) {
		do {
			if (kernel->AcquireFlush()) {
				if (TaskQueue::Flush(threadPool, priority)) {
					return true;
				} else {
					kernel->currentParallelCount.fetch_sub(1, std::memory_order_release);
					return false;
				}
			}

			size_t warp = this - &kernel->taskQueueGrid[0];
			std::atomic<size_t>& s = *reinterpret_cast<std::atomic<size_t>*>(&kernel->fullFlushMask[warp / (sizeof(size_t) * 8)]);
			s.fetch_or((size_t)1 << (warp & (sizeof(size_t) * 8 - 1)), std::memory_order_relaxed);
		} while (kernel->anyFlush.exchange(STATE_QUEUEING, std::memory_order_release) != STATE_QUEUEING);
	}

	return false;
}

#if USE_OPTICK
static const char* warpStrings[] = {
	"Warp 0", "Warp 1", "Warp 2", "Warp 3",
	"Warp 4", "Warp 5", "Warp 6", "Warp 7",
	"Warp 8", "Warp 9", "Warp 10", "Warp 11",
	"Warp 12", "Warp 13", "Warp 14", "Warp 15",
	"Warp 16", "Warp 17", "Warp 18", "Warp 19",
	"Warp 20", "Warp 21", "Warp 22", "Warp 23",
	"Warp 24", "Warp 25", "Warp 26", "Warp 27",
	"Warp 28", "Warp 29", "Warp 30", "Warp 31",
};
#endif

bool Kernel::SubTaskQueue::PreemptExecution() {
	uint32_t* expected = nullptr;
	uint32_t thisWarpIndex = verify_cast<uint32_t>(this - &kernel->taskQueueGrid[0]);
	uint32_t oldWarpIndex = WarpIndex;

	if (threadWarp.compare_exchange_strong(expected, &WarpIndex, std::memory_order_acquire)) {
#if USE_OPTICK
		const char* warpName = thisWarpIndex < sizeof(warpStrings) / sizeof(warpStrings[0]) ? warpStrings[thisWarpIndex] : "Warp X";
		OPTICK_PUSH_DYNAMIC(warpName);
#endif

		size_t warp = this - &kernel->taskQueueGrid[0];
		std::atomic<size_t>& s = *reinterpret_cast<std::atomic<size_t>*>(&kernel->idleMask[warp / (sizeof(size_t) * 8)]);
		s.fetch_and(~((size_t)1 << (warp & (sizeof(size_t) * 8 - 1))), std::memory_order_relaxed);

		WarpIndex = thisWarpIndex;
		stackQueue = oldWarpIndex;
		return true;
	} else {
		assert(WarpIndex != thisWarpIndex);
		return false;
	}
}

bool Kernel::SubTaskQueue::YieldExecution() {
	uint32_t* exp = &WarpIndex;
	if (threadWarp.compare_exchange_strong(exp, reinterpret_cast<uint32_t*>(~(size_t)0), std::memory_order_relaxed)) {
		OPTICK_POP();

		WarpIndex = stackQueue;
		stackQueue = ~(uint32_t)0;
		threadWarp.store(static_cast<uint32_t*>(nullptr), std::memory_order_release);

		size_t warp = this - &kernel->taskQueueGrid[0];
		std::atomic<size_t>& s = *reinterpret_cast<std::atomic<size_t>*>(&kernel->idleMask[warp / (sizeof(size_t) * 8)]);
		s.fetch_or((size_t)1 << (warp & (sizeof(size_t) * 8 - 1)), std::memory_order_relaxed);

		if (queueing.exchange(STATE_IDLE, std::memory_order_release) == STATE_QUEUEING) {
			Flush(kernel->threadPool);
		}

		return true;
	} else {
		return false;
	}
}

void Kernel::SubTaskQueue::ExecuteImpl(void* context) {
	if (suspendCount.load(std::memory_order_acquire) == 0) {
		PreemptGuard<false> guard(*this, verify_cast<uint32_t>(this - &kernel->taskQueueGrid[0]));
		if (guard) {
			if (suspendCount.load(std::memory_order_acquire) == 0) {
				queueing.store(STATE_FLUSHING, std::memory_order_release);
				TaskQueue::Execute(context);
				guard.Cleanup();

				if (!YieldExecution()) {
					Flush(kernel->threadPool);
				}
			} else {
				queueing.store(STATE_QUEUEING, std::memory_order_relaxed);
			}
		}
	}
}

void Kernel::SubTaskQueue::Execute(void* context) {
#ifdef _DEBUG
	InWarpRoutine = true;
#endif
	ExecuteImpl(context);
	kernel->ReleaseFlush();
#ifdef _DEBUG
	InWarpRoutine = false;
#endif
}

void Kernel::SubTaskQueue::AbortImpl(void* context) {
	TaskQueue::Abort(context);
}

void Kernel::SubTaskQueue::Abort(void* context) {
	AbortImpl(context);
	kernel->ReleaseFlush();
}

bool Kernel::SubTaskQueue::InvokeOperation(std::pair<ITask*, void*>& task, void (ITask::*operation)(void*), void* context) {
	OPTICK_EVENT();
	uint32_t thisWarpIndex = verify_cast<uint32_t>(this - &kernel->taskQueueGrid[0]);
	assert(operation == &ITask::Abort || WarpIndex == thisWarpIndex);

	WarpTiny* warpTiny = reinterpret_cast<WarpTiny*>(task.second);
	// It's possible that tiny's warp changed before Invoking.
	if (warpTiny == nullptr || thisWarpIndex == warpTiny->GetWarpIndex()) {
		// Not changed, call it at once
		(task.first->*operation)(context);

		// we have hold tiny on enqueuing, release it after calling.
		if (warpTiny != nullptr) {
#ifdef _DEBUG
			kernel->activeTaskCount.fetch_sub(1, std::memory_order_relaxed);
#endif
			warpTiny->ReleaseObject();
		}
	} else {
		// requeue it
		kernel->QueueRoutineInternal(warpTiny->GetWarpIndex(), kernel->threadPool.GetCurrentThreadIndex(), warpTiny, task.first);
	}

	// recheck suspend and yield status
	return operation == &ITask::Abort || (suspendCount.load(std::memory_order_acquire) == 0 && WarpIndex == thisWarpIndex);
}

void Kernel::SubTaskQueue::Push(uint32_t fromThreadIndex, ITask* task, WarpTiny* warpTiny) {
	// Just forward
	TaskQueue::Push(fromThreadIndex, task, warpTiny);
}

const std::vector<TaskQueue::RingBuffer>& Kernel::SubTaskQueue::GetRingBuffers() const {
	return TaskQueue::GetRingBuffers();
}

WarpYieldGuard::WarpYieldGuard(Kernel& k) : kernel(k), warp(k.GetCurrentWarpIndex()) {
	kernel.YieldCurrentWarp();
}

WarpYieldGuard::~WarpYieldGuard() {
	if (warp != ~(uint32_t)0) {
		kernel.WaitWarp(warp);
	}
}

