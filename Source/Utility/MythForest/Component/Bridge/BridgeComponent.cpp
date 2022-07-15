#include "BridgeComponent.h"
#include "../../Entity.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

BridgeComponent::BridgeComponent(const TShared<Component>& targetComponent) : hostEntity(nullptr) {
	Flag().fetch_or(TINY_UNIQUE, std::memory_order_relaxed);
	assert(targetComponent);
	assert(targetComponent->GetWarpIndex() == GetWarpIndex());
}

void BridgeComponent::Clear(Engine& engine) {
	if (hostEntity != nullptr) {
		hostEntity->RemoveComponent(engine, this);
		hostEntity = nullptr;
	}
}

void BridgeComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	// broadcast all events.
	targetComponent->DispatchEvent(event, entity);
}

void BridgeComponent::Initialize(Engine& engine, Entity* entity) {
	assert(hostEntity == nullptr);
	hostEntity = entity;
}

void BridgeComponent::Uninitialize(Engine& engine, Entity* entity) {
	Clear(engine);
}

Entity* BridgeComponent::GetHostEntity() const {
	return hostEntity;
}

