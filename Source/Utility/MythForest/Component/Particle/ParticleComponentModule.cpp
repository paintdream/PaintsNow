#include "ParticleComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(ParticleComponentModule);
ParticleComponentModule::ParticleComponentModule(Engine& engine) : BaseClass(engine) {}
ParticleComponentModule::~ParticleComponentModule() {}

TObject<IReflect>& ParticleComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
	}

	return *this;
}

TShared<ParticleComponent> ParticleComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<ParticleComponent> particleComponent = TShared<ParticleComponent>::From(allocator->New());
	particleComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return particleComponent;
}
