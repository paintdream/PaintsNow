#include "Memory.h"
#include "../../../Core/Template/TAllocator.h"
#include "../../../Core/System/ThreadPool.h"
#include "../../../Core/Driver/Thread/Pthread/ZThreadPthread.h"

using namespace PaintsNow;

bool Memory::Initialize() {
	return true;
}

struct_aligned(CPU_CACHELINE_SIZE) data {
	int32_t values[16];
};

static void TaskRoutine(const TShared<TObjectAllocator<data> >& trunks, std::vector<data*>& ptrs, size_t k, size_t length, std::atomic<uint32_t>& count, std::atomic<uint32_t>& total) {
	for (size_t i = 0; i < 77777; i++) {
		data*& p = ptrs[(rand() * length + k) % ptrs.size()];
		if (p == nullptr) {
			total.fetch_add(1, std::memory_order_relaxed);
			p = trunks->New();
		}

		data*& q = ptrs[(rand() * length + k) % ptrs.size()];
		if (q != nullptr) {
			trunks->Delete(q);
			q = nullptr;
			total.fetch_sub(1, std::memory_order_release);
		}
	}

	count.fetch_sub(1, std::memory_order_release);
}

bool Memory::Run(int randomSeed, int length) {
	TShared<TObjectAllocator<data> > trunks = TShared<TObjectAllocator<data> >::From(new TObjectAllocator<data>());
	ZThreadPthread threadApi;
	ThreadPool threadPool(threadApi, length);

	std::vector<data*> ptrs;
	ptrs.resize(8000 * length, nullptr);
	srand(randomSeed);
	std::atomic<uint32_t> count;
	std::atomic<uint32_t> total;
	total.store(0, std::memory_order_release);
	count.store(length, std::memory_order_release);

	for (size_t k = 0; k < (size_t)length; k++) {
		threadPool.Dispatch(CreateTaskContextFree(Wrap(TaskRoutine), trunks, std::ref(ptrs), k, length, std::ref(count), std::ref(total)));
	}

	while (count.load(std::memory_order_acquire) != 0) {
		YieldThread();
	}

	for (size_t n = 0; n < ptrs.size(); n++) {
		data* p = ptrs[n];
		if (p != nullptr) {
			total.fetch_sub(1, std::memory_order_release);
			trunks->Delete(p);
		}
	}

	getchar();
	return true;
}

void Memory::Summary() {
}

TObject<IReflect>& Memory::operator ()(IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}