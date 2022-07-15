#include "SpaceComponentModule.h"
#include "../../Entity.h"
#include "../../../BridgeSunset/BridgeSunset.h"

using namespace PaintsNow;

// Script interfaces
CREATE_MODULE(SpaceComponentModule);
SpaceComponentModule::SpaceComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& SpaceComponentModule::operator ()(IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestInsertEntity)[ScriptMethodLocked = "InsertEntity"];
		ReflectMethod(RequestRemoveEntity)[ScriptMethodLocked = "RemoveEntity"];
		ReflectMethod(RequestQueryEntities)[ScriptMethod = "QueryEntities"];
		ReflectMethod(RequestSetForwardMask)[ScriptMethodLocked = "SetForwardMask"];
		ReflectMethod(RequestGetEntityCount)[ScriptMethodLocked = "GetEntityCount"];
		ReflectMethod(RequestOptimize)[ScriptMethodLocked = "Optimize"];
	}

	return *this;
}

TShared<SpaceComponent> SpaceComponentModule::RequestNew(IScript::Request& request, int32_t warpIndex, bool sorted) {
	TShared<SpaceComponent> spaceComponent = TShared<SpaceComponent>::From(allocator->New(sorted));
	uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
	if (warpIndex < 0) {
		assert(currentWarpIndex != ~(uint32_t)0);
		spaceComponent->SetWarpIndex(currentWarpIndex);
	} else {
		spaceComponent->SetWarpIndex(warpIndex);
		if (warpIndex != currentWarpIndex) {
			spaceComponent->Flag().fetch_or(Component::COMPONENT_OVERRIDE_WARP);
		}
	}

	return spaceComponent;
}

void SpaceComponentModule::RequestSetForwardMask(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, uint32_t forwardMask) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);

	if (forwardMask != 0) {
		spaceComponent->Flag().fetch_or(SpaceComponent::SPACECOMPONENT_FORWARD_EVENT_TICK, std::memory_order_relaxed);
	} else {
		spaceComponent->Flag().fetch_and(~SpaceComponent::SPACECOMPONENT_FORWARD_EVENT_TICK, std::memory_order_relaxed);
	}
}

uint32_t SpaceComponentModule::RequestGetEntityCount(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);
	return spaceComponent->GetEntityCount();
}

void SpaceComponentModule::RequestOptimize(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);

	spaceComponent->Optimize();
}

void SpaceComponentModule::RequestInsertEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	if (spaceComponent->Flag().load(std::memory_order_acquire) & SpaceComponent::TINY_ACTIVATED) {
		spaceComponent->Insert(engine, entity.Get());
	} else {
		request.Error("Orphan SpaceComponent cannot hold entities.");
	}
}

void SpaceComponentModule::RequestRemoveEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	spaceComponent->Remove(engine, entity.Get());
}

void SpaceComponentModule::RequestQueryEntities(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, const Float3Pair& box) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);

	std::vector<TShared<Entity> > entityList;
	spaceComponent->QueryEntities(entityList, box);

	request.DoLock();
	for (size_t i = 0; i < entityList.size(); i++) {
		request << entityList[i];
	}
	request.UnLock();
}

void SpaceComponentModule::ScriptUninitialize(IScript::Request& request) {
	Module::ScriptUninitialize(request);
}
