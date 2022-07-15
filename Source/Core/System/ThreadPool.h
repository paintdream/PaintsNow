// ThreadPool.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once
#include "../Interface/IThread.h"
#include "../Interface/ITask.h"
#include "../Template/TProxy.h"
#include "../Template/TAtomic.h"
#include <vector>

namespace PaintsNow {
	// ThreadPool with fixed worker count
	class ThreadPool : public ISyncObject {
	public:
		ThreadPool(IThread& threadApi, uint32_t threadCount, long balanceWindowSize = 8);

	public:
		~ThreadPool() override;

		// Dispatch a task. Notice that a task will be ignored if it has been pushed before and not be executed yet.
		// if (priority >= 0): only thread with index >= priority can poll this task
		// else: only thread with index >= (threadCount + priority) can poll this task
		bool Dispatch(ITask* task, int priority = 0);
		uint32_t GetThreadCount() const;
		uint32_t GetTaskCount() const;
		static uint32_t GetCurrentThreadIndex();

		// Custom thread context
		void SetThreadContext(uint32_t index, void* context);
		void* GetThreadContext(uint32_t index) const;

		// Pool specified thread task on current thread. (can be run with non-pooled thread)
		bool Poll(uint32_t index);
		bool PollDelay(uint32_t index, uint32_t delayMilliseconds = 5);
		bool PollWait(uint32_t index, std::atomic<uint32_t>& variable, uint32_t mask = ~(uint32_t)0, uint32_t flag = 0, uint32_t delay = 5);
		uint32_t PollExchange(uint32_t index, std::atomic<uint32_t>& variable, uint32_t value, uint32_t delay = 5);
		bool PollCompareExchange(uint32_t index, std::atomic<uint32_t>& variable, uint32_t mask, uint32_t flag, uint32_t delay = 5);
		bool IsRunning() const;

		IThread& GetThreadApi();
		void Initialize();
		void Uninitialize();
		bool IsInitialized() const;
		void Limit(uint32_t offset);
		void BalanceUp();
		void BalanceDown();

	protected:
		bool Run(IThread::Thread* thread, size_t threadID);
		size_t Fetch(size_t index);
		virtual void Signal();
		virtual void Wait(uint32_t delay);

	protected:
		alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> liveThreadCount;
		alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> runningToken;
		alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> runningCount;
		alignas(CPU_CACHELINE_SIZE) size_t waitEventCounter;
		alignas(CPU_CACHELINE_SIZE) std::atomic<ptrdiff_t> balanceCounter;
		alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> taskCount;
		
		size_t threadCount;
		size_t limit;
		ptrdiff_t balanceWindowSize;
		IThread::Event* eventPump;

		struct_aligned(CPU_CACHELINE_SIZE) ThreadInfo {
			std::atomic<ITask*>& TaskHead() {
				return *reinterpret_cast<std::atomic<ITask*>*>(&taskHead);
			}

			std::atomic<size_t>& TaskTicket() {
				return *reinterpret_cast<std::atomic<size_t>*>(&taskTicket);
			}

		private:
			ITask* taskHead;
			size_t taskTicket;

		public:
			void* context;
			IThread::Thread* threadHandle;
		};

		std::vector<ThreadInfo> threadInfos;
	};
}
