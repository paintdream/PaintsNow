#include "TaskAllocator.h"
#include "../../../Core/System/ThreadPool.h"
#include "../../../Core/Driver/Thread/Pthread/ZThreadPthread.h"

using namespace PaintsNow;

bool TaskAllocator::Initialize() {
	return true;
}

static void TaskRoutine(ThreadPool& threadPool, int count, std::atomic<size_t>& counter) {
	for (int i = 0; i < count; i++) {
		counter.fetch_add(1);
		threadPool.Dispatch(CreateTaskContextFree(Wrap(TaskRoutine), std::ref(threadPool), rand() % (count / 8 + 1), std::ref(counter)));
	}
	
	counter.fetch_sub(1);
}

bool TaskAllocator::Run(int randomSeed, int length) {
	ZThreadPthread threadApi;
	ThreadPool threadPool(threadApi, length);
	std::atomic<size_t> counter;
	counter.store(0, std::memory_order_relaxed);

	srand(randomSeed);
	for (size_t k = 0; k < (size_t)length * 64; k++) {
		counter.fetch_add(1);
		threadPool.Dispatch(CreateTaskContextFree(Wrap(TaskRoutine), std::ref(threadPool), rand() % 256, std::ref(counter)));
	}

	while (counter.load() != 0) {
		printf("TaskAllocator running %d.\n", (int)counter.load());
		threadApi.Sleep(200);
	}
	
	printf("TaskAllocator running %d.\n", (int)counter.load());
	getchar();
	return true;
}

void TaskAllocator::Summary() {}

TObject<IReflect>& TaskAllocator::operator ()(IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}