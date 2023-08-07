#include "TaskQueue.h"
#include "ThreadPool.h"
using namespace PaintsNow;

TaskQueue::TaskQueue(uint32_t idCount) : barrierVersion(0), currentVersion(0), threadIndex(0) {
	ringBuffers.resize(idCount);
	ringVersions.resize(idCount);
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
	size_t executeCounter;
	do {
		executeCounter = 0;
		size_t stepVersion = currentVersion;
		for (size_t i = 0; i < ringBuffers.size(); i++) {
			RingBuffer& ringBuffer = ringBuffers[i];
			size_t& counter = ringVersions[i];

			while (static_cast<ptrdiff_t>(currentVersion - counter) >= 0 && !ringBuffer.Empty()) {
				std::pair<ITask*, void*> task = ringBuffer.Top();
				ringBuffer.Pop();
				executeCounter++;

				if (task.first == nullptr) { // barrier
					counter = reinterpret_cast<size_t>(task.second);
				} else if (!InvokeOperation(task, operation, context)) {
					return;
				}

				if (currentVersion + 1 == counter) {
					stepVersion = counter;
				} else if (static_cast<ptrdiff_t>(currentVersion - counter) > 0) {
					counter = currentVersion;
				}
			}
		}

		currentVersion = stepVersion;
	} while (executeCounter != 0);
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

void TaskQueue::Barrier(uint32_t id) {
	assert(id < verify_cast<uint32_t>(ringBuffers.size()));
	RingBuffer& ringBuffer = ringBuffers[id];
	size_t version = reinterpret_cast<std::atomic<size_t>&>(barrierVersion).fetch_add(1, std::memory_order_acquire) + 1;
	ringBuffer.Push(std::make_pair(static_cast<ITask*>(nullptr), reinterpret_cast<void*>(version)));
}