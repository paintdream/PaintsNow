// TaskQueue.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-30
//

#pragma once
#include "../Template/TAtomic.h"
#include "../Template/TQueue.h"
#include "../Interface/ITask.h"
#include "../Interface/IType.h"
#include <vector>

namespace PaintsNow {
	// TaskQueue is single-thread read + single-thread/write task ring buffer
	// All tasks in the same task queue are scheduled in sequence. So no lock is needed for themselves ...
	// TaskQueue is also an ITask.
	class ThreadPool;
	class TaskQueue : public ITask {
	public:
		TaskQueue(uint32_t idCount);
		~TaskQueue() override;
		void Push(uint32_t id, ITask* task, void* tag);
		bool Flush(ThreadPool& threadPool, int priority);
		typedef TQueueList<std::pair<ITask*, void*> > RingBuffer;
		const std::vector<RingBuffer>& GetRingBuffers() const;

	protected:
		virtual bool InvokeOperation(std::pair<ITask*, void*>& task, void (ITask::*operation)(void*), void* context);

		void Execute(void* context) override;
		void Suspend(void* context) final;
		void Resume(void* context) final;
		void Abort(void* context) override;
		bool Continue() const final;

	protected:
		void OnOperation(void (ITask::*operation)(void*), void* context);

		// Wait-free Ring Buffer
		std::vector<RingBuffer> ringBuffers;
		size_t threadIndex;
	};
}

