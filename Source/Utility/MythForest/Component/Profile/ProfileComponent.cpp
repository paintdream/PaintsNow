#include "ProfileComponent.h"
#include "../../Entity.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

ProfileComponent::ProfileComponent(float ratio) : timeStamp(0), tickInterval(0), historyRatio(ratio) {}

Tiny::FLAG ProfileComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT;
}

float ProfileComponent::GetTickInterval() const {
	return tickInterval;
}

void ProfileComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
}
