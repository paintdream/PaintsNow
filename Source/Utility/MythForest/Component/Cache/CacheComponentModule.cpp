#include "CacheComponentModule.h"
#include "CacheComponent.h"

using namespace PaintsNow;

CREATE_MODULE(CacheComponentModule);
CacheComponentModule::CacheComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& CacheComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestPushObjects)[ScriptMethodLocked = "PushObjects"];
		ReflectMethod(RequestClearObjects)[ScriptMethodLocked = "ClearObjects"];
	}

	return *this;
}

TShared<CacheComponent> CacheComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<CacheComponent> cacheComponent = TShared<CacheComponent>::From(allocator->New());
	cacheComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return cacheComponent;
}

void CacheComponentModule::RequestPushObjects(IScript::Request& request, IScript::Delegate<CacheComponent> cacheComponent, std::vector<IScript::Delegate<SharedTiny> >& objects) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(cacheComponent);
	CHECK_THREAD_IN_MODULE(cacheComponent);

	std::vector<TShared<SharedTiny> > converted(objects.size());
	for (size_t i = 0; i < objects.size(); i++) {
		converted[i] = objects[i].Get();
	}

	cacheComponent->PushObjects(std::move(converted));
}

void CacheComponentModule::RequestClearObjects(IScript::Request& request, IScript::Delegate<CacheComponent> cacheComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(cacheComponent);
	CHECK_THREAD_IN_MODULE(cacheComponent);

	cacheComponent->ClearObjects();
}
