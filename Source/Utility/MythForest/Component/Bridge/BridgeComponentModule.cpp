#include "BridgeComponentModule.h"
#include "../../Engine.h"
#include "../../../HeartVioliner/Clock.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include <iterator>

using namespace PaintsNow;

CREATE_MODULE(BridgeComponentModule);
BridgeComponentModule::BridgeComponentModule(Engine& engine) : BaseClass(engine) {
}

TObject<IReflect>& BridgeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
	}

	return *this;
}

TShared<BridgeComponent> BridgeComponentModule::New(const TShared<Component>& component) {
	TShared<BridgeComponent> bridgeComponent = TShared<BridgeComponent>::From(allocator->New(component));
	bridgeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return bridgeComponent;
}

TShared<BridgeComponent> BridgeComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(component);

	return New(component.Get());
}
