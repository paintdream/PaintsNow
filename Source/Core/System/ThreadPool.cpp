#define MAX_YIELD_COUNT 8

#include "ThreadPool.h"
#include "../Template/TProxy.h"
#include "../Driver/Profiler/Optick/optick.h"
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <sstream>

using namespace PaintsNow;

ThreadPool::ThreadPool(IThread& t, uint32_t tc, long bw) : ISyncObject(t), threadCount(tc), balanceWindowSize(bw) {
	assert(threadCount != 0);
	eventPump = threadApi.NewEvent();
	Initialize();
}

bool ThreadPool::IsInitialized() const {
	return !threadInfos.empty();
}

void ThreadPool::Limit(uint32_t offset) {
	limit = offset;
}

void ThreadPool::BalanceUp() {
	if (limit != 0 && taskCount.load(std::memory_order_acquire) > 0) {
		ptrdiff_t size = balanceCounter.load(std::memory_order_acquire);
		if (size > balanceWindowSize) {
			Limit((uint32_t)--limit);
			balanceCounter.fetch_sub(balanceWindowSize, std::memory_order_relaxed);
		} else {
			balanceCounter.fetch_add(1, std::memory_order_relaxed);
		}
	}
}

void ThreadPool::BalanceDown() {
	if (limit + 1 < threadCount && taskCount.load(std::memory_order_acquire)  == 0) {
		ptrdiff_t size = balanceCounter.load(std::memory_order_acquire);
		if (size + balanceWindowSize < 0) {
			Limit((uint32_t)++limit);
			balanceCounter.fetch_add(balanceWindowSize, std::memory_order_relaxed);
		} else {
			balanceCounter.fetch_sub(1, std::memory_order_relaxed);
		}
	}
}

void ThreadPool::Initialize() {
	// Initialize thread pool states.
	liveThreadCount.store(0, std::memory_order_relaxed);
	runningCount.store(0, std::memory_order_relaxed);
	runningToken.store(1, std::memory_order_relaxed);
	balanceCounter.store(0, std::memory_order_relaxed);
	taskCount.store(0, std::memory_order_relaxed);
	waitEventCounter = 0;
	limit = 0;

	assert(threadInfos.empty());
	threadInfos.resize(threadCount);

	// create thread workers
	for (size_t i = 0; i < threadInfos.size(); i++) {
		ThreadInfo& info = threadInfos[i];
		info.context = nullptr;
		info.TaskHead().store(nullptr, std::memory_order_relaxed);
	}

	std::atomic_thread_fence(std::memory_order_release);

	for (size_t k = 0; k < threadInfos.size(); k++) {
		ThreadInfo& info = threadInfos[k];
		info.threadHandle = threadApi.NewThread(Wrap(this, &ThreadPool::Run), k);
		char threadName[64];
		sprintf(threadName, "ThreadPool Worker %d", (int)k);
		threadApi.SetThreadName(info.threadHandle, threadName);
	}
}

uint32_t ThreadPool::GetThreadCount() const {
	return verify_cast<uint32_t>(threadInfos.size());
}

uint32_t ThreadPool::GetTaskCount() const {
	return verify_cast<uint32_t>(taskCount.load(std::memory_order_acquire));
}

thread_local uint32_t localThreadIndex = ~(uint32_t)0;

uint32_t ThreadPool::GetCurrentThreadIndex() {
	return localThreadIndex;
}

IThread& ThreadPool::GetThreadApi() {
	return threadApi;
}

void ThreadPool::Signal() {
	threadApi.DoLock(mutex);
	threadApi.Signal(eventPump);
	threadApi.UnLock(mutex);
}

void ThreadPool::Wait(uint32_t delay) {
	threadApi.DoLock(mutex);
	++waitEventCounter;
	if (delay == ~(uint32_t)0) {
		threadApi.Wait(eventPump, mutex);
	} else {
		threadApi.Wait(eventPump, mutex, delay);
	}
	--waitEventCounter;
	threadApi.UnLock(mutex);
}

void ThreadPool::SetThreadContext(uint32_t id, void* context) {
	assert(id < threadInfos.size());
	threadInfos[id].context = context;
}

void* ThreadPool::GetThreadContext(uint32_t id) const {
	assert(id < threadInfos.size());
	return threadInfos[id].context;
}

void ThreadPool::Uninitialize() {
	// force kill all pending tasks
	size_t threadCount = threadInfos.size();
	assert(threadCount != 0);

	DoLock();
	runningToken.store(0, std::memory_order_release);

	if (threadCount != 0) {
		const uint32_t WAIT_DELAY = 200;
		uint32_t yieldCount = 0;
		// wait for live thread exit
		while (true) {
			size_t count = liveThreadCount.load(std::memory_order_acquire);
			if (count == 0)
				break;

			UnLock();
			for (size_t i = 0; i < count; i++) {
				Signal();
			}

			Wait(WAIT_DELAY);
			DoLock();
		}

		for (size_t k = 0; k < threadCount; k++) {
			ThreadInfo& info = threadInfos[k];
			std::atomic<ITask*>& taskHead = info.TaskHead();

			ITask* p = taskHead.exchange(nullptr, std::memory_order_acquire);
			while (p != nullptr) {
				ITask* q = p;
				p = p->next;
				q->next = nullptr;
				q->Abort(nullptr);
			}

			threadApi.DeleteThread(info.threadHandle);
		}

		threadInfos.clear();
		std::atomic_thread_fence(std::memory_order_release);
	}

	UnLock();
}

ThreadPool::~ThreadPool() {
	if (!threadInfos.empty()) {
		Uninitialize();
	}

	threadApi.DeleteEvent(eventPump);
}

bool ThreadPool::Dispatch(ITask* task, int priority) {
	assert(task != nullptr);
	if (runningToken.load(std::memory_order_acquire) != 0) {
		std::atomic<size_t>& queued = *reinterpret_cast<std::atomic<size_t>*>(&task->queued);
		if (queued.exchange(1, std::memory_order_acquire) == 1) // already pushed
			return false;

		taskCount.fetch_add(1, std::memory_order_relaxed);
		int offset = priority >= 0 ? priority : (int)GetThreadCount() + priority;
		size_t index = offset < 0 ? 0u : Math::Min((uint32_t)offset, GetThreadCount() - 1);
		std::atomic<ITask*>& taskHead = threadInfos[index].TaskHead();

		// Chain task
		assert(task != taskHead.load(std::memory_order_acquire));
		assert(task->next == nullptr);
		ITask* r = taskHead.load(std::memory_order_relaxed);
		do {
			task->next = r;
		} while (!taskHead.compare_exchange_weak(r, task, std::memory_order_acq_rel, std::memory_order_relaxed));

		if (waitEventCounter > index + limit) {
			Signal();
		}

		return true;
	} else {
		task->Abort(nullptr);
		return false;
	}
}

inline size_t ThreadPool::Fetch(size_t maxIndex) {
	for (size_t n = 0; n < maxIndex; n++) {
		if (threadInfos[n].TaskHead().load(std::memory_order_acquire) != nullptr) {
			return n;
		}
	}

	return ~(size_t)0;
}

bool ThreadPool::Poll(uint32_t index) {
	OPTICK_EVENT();
	size_t priorityIndex = runningCount.load(std::memory_order_acquire);

	// Wait for a moment
	size_t maxIndex = threadCount - Math::Min(priorityIndex, threadCount);
	size_t priority = Fetch(maxIndex);
	bool ret;

	if (priority != ~(size_t)0) {
		size_t k = runningCount.fetch_add(1, std::memory_order_relaxed);
		if (priority + k < threadCount) {
			std::atomic<ITask*>& taskHead = threadInfos[priority].TaskHead();

			// Has task?
			if (taskHead.load(std::memory_order_acquire) != nullptr) {
				ITask* p = taskHead.exchange(nullptr, std::memory_order_acquire);
				if (p != nullptr) {
					ITask* next = p->next;

					if (next != nullptr) {
						p->next = nullptr;
						ITask* t = taskHead.exchange(next, std::memory_order_release);

						// Someone has pushed some new tasks at the same time.
						// So rechain remaining tasks proceeding to the current one to new task head atomically.
						if (t != nullptr) {
							do {
								ITask* q = t;
								assert(q->queued == 1);
								t = t->next;
								ITask* r = taskHead.load(std::memory_order_relaxed);

								do {
									q->next = r;
								} while (!taskHead.compare_exchange_weak(r, q, std::memory_order_relaxed, std::memory_order_relaxed));
							} while (t != nullptr);

							std::atomic_thread_fence(std::memory_order_acq_rel);

							if (waitEventCounter > priority + limit) {
								Signal();
							}
						}
					}

					taskCount.fetch_sub(1, std::memory_order_relaxed);

					assert(p->next == nullptr);
					std::atomic<size_t>& queued = *reinterpret_cast<std::atomic<size_t>*>(&p->queued);
					queued.store(0, std::memory_order_release);

					// OK. now we can execute the task
					void* context = threadInfos[index].context;

					OPTICK_PUSH("Execute");

					// Exited?
					if (runningToken.load(std::memory_order_relaxed) == 0) {
						p->Abort(context);
					} else {
						p->Execute(context);
					}

					OPTICK_POP();
				}
			} else {
				YieldThreadFast();
			}

			ret = true;
		} else {
			ret = false;
		}

		runningCount.fetch_sub(1, std::memory_order_release);
	} else {
		ret = false;
	}

	return ret;
}

bool ThreadPool::IsRunning() const {
	return runningToken.load(std::memory_order_acquire) != 0;
}

bool ThreadPool::PollDelay(uint32_t index, uint32_t delay) {
	if (delay == 0) {
		return Poll(index);
	} else {
		if (!Poll(index) && runningToken.load(std::memory_order_acquire) != 0) {
			Wait(delay);

			return false;
		} else {
			return true;
		}
	}
}

bool ThreadPool::PollWait(uint32_t threadIndex, std::atomic<uint32_t>& variable, uint32_t mask, uint32_t flag, uint32_t delay) {
	bool result;
	while ((result = ((variable.load(std::memory_order_acquire) & mask) != flag)) && IsRunning()) {
		PollDelay(threadIndex, delay);
	}

	return !result;
}

uint32_t ThreadPool::PollExchange(uint32_t threadIndex, std::atomic<uint32_t>& variable, uint32_t value, uint32_t delay) {
	uint32_t target;
	while ((target = variable.exchange(value, std::memory_order_acq_rel)) == value && IsRunning()) {
		PollDelay(threadIndex, delay);
	}

	return target;
}

bool ThreadPool::PollCompareExchange(uint32_t threadIndex, std::atomic<uint32_t>& variable, uint32_t mask, uint32_t flag, uint32_t delay) {
	uint32_t current = variable.load(std::memory_order_acquire);
	uint32_t target;
	bool result = false;

	while (IsRunning()) {
		target = (current & ~mask) | ((current & mask) & flag);
		if (!(result = variable.compare_exchange_weak(current, target, std::memory_order_release))) {
			PollDelay(threadIndex, delay);
		} else {
			break;
		}
	}

	return result;
}

bool ThreadPool::Run(IThread::Thread* thread, size_t index) {
	std::stringstream ss;
	ss << "Worker " << index;
	OPTICK_THREAD(ss.str().c_str());

	// set thread local
	localThreadIndex = verify_cast<uint32_t>(index);
	// fetch one and execute
	liveThreadCount.fetch_add(1, std::memory_order_acquire);
	while (runningToken.load(std::memory_order_acquire) != 0) {
		if (!Poll(verify_cast<uint32_t>(index))) {
			Wait(~(uint32_t)0);
		}
	}

	liveThreadCount.fetch_sub(1, std::memory_order_release);
	return false; // manages IThread::Thread* by ourself
}
