#include "ExplorerComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(ExplorerComponentModule);
ExplorerComponentModule::ExplorerComponentModule(Engine& engine) : BaseClass(engine) {}
ExplorerComponentModule::~ExplorerComponentModule() {}

TObject<IReflect>& ExplorerComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetProxyConfig)[ScriptMethodLocked = "SetProxyConfig"];
	}

	return *this;
}

TShared<ExplorerComponent> ExplorerComponentModule::RequestNew(IScript::Request& request, const String& identifier) {
	CHECK_REFERENCES_NONE();
	TShared<ExplorerComponent> explorerComponent = TShared<ExplorerComponent>::From(allocator->New(Unique(identifier)));
	explorerComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return explorerComponent;
}

void ExplorerComponentModule::RequestSetProxyConfig(IScript::Request& request, IScript::Delegate<ExplorerComponent> explorerComponent, IScript::Delegate<Component> component, float activateThreshold, float deactivateThreshold) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(explorerComponent);
	CHECK_DELEGATE(component);

	ExplorerComponent::ProxyConfig config;
	config.activateThreshold = activateThreshold;
	config.deactivateThreshold = deactivateThreshold;

	explorerComponent->SetProxyConfig(component.Get(), config);
}
