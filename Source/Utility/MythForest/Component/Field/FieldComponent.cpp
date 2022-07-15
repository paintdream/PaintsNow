#include "FieldComponent.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
using namespace PaintsNow;

FieldComponent::FieldComponent() : subType(0) {
}

FieldComponent::~FieldComponent() {}

TObject<IReflect>& FieldComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(fieldImpl);
	}

	return *this;
}

Bytes FieldComponent::operator [] (const Float3& position) const {
	return (*fieldImpl)[position];
}

void FieldComponent::SetField(const TShared<FieldBase>& field) {
	fieldImpl = field;
}

void FieldComponent::PostEvent(SpaceComponent* spaceComponent, Event& event, FLAG mask) const {
	assert(spaceComponent != nullptr);
	assert(fieldImpl);
	assert(spaceComponent->GetWarpIndex() == GetWarpIndex());

	Entity* rootEntity = spaceComponent->GetRootEntity();
	if (rootEntity != nullptr) {
		fieldImpl->PostEventForEntityTree(rootEntity, event, mask);
	}
}

void FieldComponent::QueryEntities(SpaceComponent* spaceComponent, std::vector<TShared<Entity> >& entities) const {
	OPTICK_EVENT();
	assert(spaceComponent != nullptr);
	assert(fieldImpl);
	assert(spaceComponent->GetWarpIndex() == GetWarpIndex());

	Entity* rootEntity = spaceComponent->GetRootEntity();
	if (rootEntity != nullptr) {
		fieldImpl->QueryEntitiesForEntityTree(rootEntity, entities);
	}
}

// Trivial implementation with operator []
void FieldComponent::FieldBase::PostEventForEntityTree(Entity* entity, Event& event, FLAG mask) const {
	assert(entity != nullptr);
	for (Entity* p = entity; p != nullptr; p = p->Right()) {
		// Query center only ...
		const Float3Pair& box = p->GetKey();
		Float3 center = (box.second + box.first) * 0.5f;
		Bytes result = (*this)[center];
		if (!result.Empty() && *result.GetData() != 0) {
			p->PostEvent(event, mask);
		}

		// iterate next entities ...
		Entity* left = p->Left();
		if (left != nullptr) {
			PostEventForEntityTree(left, event, mask);
		}
	}
}

void FieldComponent::FieldBase::QueryEntitiesForEntityTree(Entity* entity, std::vector<TShared<Entity> >& entities) const {
	assert(entity != nullptr);
	for (Entity* p = entity; p != nullptr; p = p->Right()) {
		// Query center only ...
		const Float3Pair& box = p->GetKey();
		Float3 center = (box.second + box.first) * 0.5f;
		Bytes result = (*this)[center];
		if (!result.Empty() && *result.GetData() != 0) {
			entities.emplace_back(p);
		}

		// iterate next entities ...
		Entity* left = p->Left();
		if (left != nullptr) {
			QueryEntitiesForEntityTree(left, entities);
		}
	}
}
