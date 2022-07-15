#include "ReflectTrail.h"
using namespace PaintsNow;

class TrailSymbolReflector : public IReflect {
public:
	TrailSymbolReflector(ReflectTrail& trail) : IReflect(true, false, false, false), reflectTrail(trail) {
		reflectTrail.AddSymbol(0, sizeof(size_t), "void*", "__vtable");
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);

private:
	ReflectTrail& reflectTrail;
};

void TrailSymbolReflector::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	size_t offset = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(base);
	if (offset >= 0 && offset < reflectTrail.GetTypeSize()) {
		reflectTrail.AddSymbol(offset, typeID->GetSize(), typeID->GetBriefName(), name);
	}
}

ReflectTrail::ReflectTrail(IThread& threadApi, const IReflectObjectComplex& prototype, size_t maxEventCount, size_t mult, size_t maxCount) : thread(threadApi), LeavesTrail(prototype.GetUnique()->GetBriefName(), prototype.GetUnique()->GetSize(), prototype.GetUnique()->GetAlignment(), maxEventCount), tickMultiplier(mult), maxSleepCount(maxCount) {
	TrailSymbolReflector reflector(*this);

	const_cast<IReflectObjectComplex&>(prototype)(reflector);
	if (tickMultiplier != 0) {
		tickThread = thread.NewThread(Wrap(this, &ReflectTrail::OnThreadTick), 0);
	}
}

ReflectTrail::~ReflectTrail() {
	if (tickThread != nullptr) {
		IThread::Thread* t = tickThread;
		tickThread = nullptr;
		std::atomic_thread_fence(std::memory_order_release);
		thread.Wait(t);
		thread.DeleteThread(t);
	}
}

bool ReflectTrail::OnThreadTick(IThread::Thread* curThread, size_t index) {
	size_t lastIndex = eventIndex.load(std::memory_order_acquire);

	size_t sleepCount = 0;
	while (tickThread != nullptr) {
		if (status.load(std::memory_order_acquire) == 0) {
			thread.Sleep(200);
		} else {
			size_t index = eventIndex.load(std::memory_order_acquire);
			if (index > events.size()) {
				Stop();
			} else {
				size_t varCount = typeSize / sizeof(size_t);
				if (index - lastIndex > varCount * varCount * tickMultiplier || sleepCount++ > maxSleepCount) {
					Tick();
					sleepCount = 0;
					lastIndex = index;
				} else {
					thread.Sleep(0);
				}
			}
		}
	}

	return false;
}
