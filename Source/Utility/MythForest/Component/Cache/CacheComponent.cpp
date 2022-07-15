#include "CacheComponent.h"

using namespace PaintsNow;

CacheComponent::CacheComponent() {}

void CacheComponent::PushObjects(rvalue<std::vector<TShared<SharedTiny> > > objects) {
	if (cachedObjects.empty()) {
		cachedObjects = std::move(objects);
	} else {
		std::vector<TShared<SharedTiny> >& objs = objects;
		for (size_t i = 0; i < objs.size(); i++) {
			cachedObjects.emplace_back(objs[i]);
		}
	}
}

void CacheComponent::ClearObjects() {
	cachedObjects.clear();
}