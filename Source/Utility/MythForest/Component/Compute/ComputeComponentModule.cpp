#include "ComputeComponentModule.h"

using namespace PaintsNow;

CREATE_MODULE(ComputeComponentModule);
ComputeComponentModule::ComputeComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& ComputeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
	}

	return *this;
}

TShared<ComputeComponent> ComputeComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<ComputeComponent> computeComponent = TShared<ComputeComponent>::From(allocator->New());
	computeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return computeComponent;
}

