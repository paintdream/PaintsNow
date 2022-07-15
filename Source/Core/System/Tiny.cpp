#include "Tiny.h"
#include "ThreadPool.h"

using namespace PaintsNow;

TObject<IReflect>& Tiny::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// static_assert(sizeof(flag) == sizeof(uint32_t), "Unsupported compiler.");
		ReflectProperty(flag)[Runtime];
	}

	return *this;
}

#ifdef _DEBUG
#include "../Template/TMap.h"

static std::map<SharedTiny*, SharedTiny*> TinyTree;
static std::atomic<size_t> TinyCritical;

static inline SharedTiny*& TinyFind(SharedTiny* tiny) {
	SharedTiny*& t = TinyTree[tiny];
	return t == nullptr ? t = tiny : t == tiny ? t : TinyFind(t);
}

void SharedTiny::DebugAttach(SharedTiny* host, SharedTiny* tiny) {
	TSpinLockGuard<size_t> guard(TinyCritical);
	SharedTiny*& src = TinyFind(host);
	SharedTiny*& dst = TinyFind(tiny);
	assert(src != dst);
	src = dst;
}

void SharedTiny::DebugDetach(SharedTiny* tiny) {
	TSpinLockGuard<size_t> guard(TinyCritical);
	std::map<SharedTiny*, SharedTiny*>::iterator it = TinyTree.find(tiny);
	if (it != TinyTree.end()) {
		assert((*it).second == tiny); // must be root
		TinyTree.erase(it);
	}
}

#endif // _DEBUG
