#include "Module.h"
#include "Engine.h"
#include "../../General/Interface/Interfaces.h"

using namespace PaintsNow;

Module::Module(Engine& e) : engine(e) {}

void Module::TickFrame() {}

Unique Module::GetTinyUnique() const {
	assert(false);
	return UniqueType<SharedTiny>::Get();
}

Component* Module::GetEntityUniqueComponent(Entity* entity) const {
	return nullptr;
}

TObject<IReflect>& Module::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}