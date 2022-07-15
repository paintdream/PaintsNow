#include "TaskQueue.h"
#include "ThreadPool.h"
using namespace PaintsNow;

TaskQueue::TaskQueue(uint32_t idCount) : threadIndex(0) {
	ringBuffers.resize(idCount);
}

TaskQueue::~TaskQueue() {
	// abort all
	Abort(nullptr);
}

bool TaskQueue::Flush(ThreadPool& threadPool, int priority) {
	return threadPool.Dispatch(this, priority);
}

const std::vector<TaskQueue::RingBuffer>& TaskQueue::GetRingBuffers() const {
	return ringBuffers;
}

void TaskQueue::OnOperation(void (ITask::*operation)(void*), void* context) {
	// iterate all ring buffers and do corresponding operations
	for (size_t i = 0; i < ringBuffers.size(); i++) {
		RingBuffer& ringBuffer = ringBuffers[i];
		while (!ringBuffer.Empty()) {
			std::pair<ITask*, void*> task = ringBuffer.Top();
			ringBuffer.Pop();
			if (!InvokeOperation(task, operation, context)) {
				return;
			}
		}
	}
}

bool TaskQueue::InvokeOperation(std::pair<ITask*, void*>& task, void (ITask::*operation)(void*), void* context) {
	(task.first->*operation)(context);
	return true;
}

void TaskQueue::Execute(void* context) {
	OnOperation(&ITask::Execute, context);
}

void TaskQueue::Suspend(void* context) {}

void TaskQueue::Resume(void* context) {}

void TaskQueue::Abort(void* context) {
	OnOperation(&ITask::Abort, context);
}

bool TaskQueue::Continue() const {
	return true;
}

void TaskQueue::Push(uint32_t id, ITask* task, void* tag) {
	assert(id < verify_cast<uint32_t>(ringBuffers.size()));
	RingBuffer& ringBuffer = ringBuffers[id];
	assert(task != nullptr);
	ringBuffer.Push(std::make_pair(task, tag));
}