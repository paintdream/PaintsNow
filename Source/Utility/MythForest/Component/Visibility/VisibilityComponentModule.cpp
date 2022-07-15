#include "VisibilityComponentModule.h"

using namespace PaintsNow;

CREATE_MODULE(VisibilityComponentModule);
VisibilityComponentModule::VisibilityComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& VisibilityComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetup)[ScriptMethodLocked = "Setup"];
	}

	return *this;
}

TShared<VisibilityComponent> VisibilityComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(streamComponent);

	TShared<VisibilityComponent> visibilityComponent = TShared<VisibilityComponent>::From(allocator->New(streamComponent.Get()));
	visibilityComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return visibilityComponent;
}

void VisibilityComponentModule::RequestSetup(IScript::Request& request, IScript::Delegate<VisibilityComponent> visibilityComponent, float maxDistance, const Float3& gridSize, uint32_t taskCount, uint32_t pixelThreshold, const UShort2& resolution) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(visibilityComponent);
	CHECK_THREAD_IN_MODULE(visibilityComponent);

	visibilityComponent->Setup(engine, maxDistance, gridSize, taskCount, pixelThreshold, resolution);
}

