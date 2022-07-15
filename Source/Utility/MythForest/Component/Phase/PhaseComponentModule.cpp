#include "PhaseComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(PhaseComponentModule);
PhaseComponentModule::PhaseComponentModule(Engine& engine) : BaseClass(engine) {}
PhaseComponentModule::~PhaseComponentModule() {}

TObject<IReflect>& PhaseComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetup)[ScriptMethod = "Setup"];
		ReflectMethod(RequestUpdate)[ScriptMethod = "Update"];
		ReflectMethod(RequestStep)[ScriptMethod = "Step"];
		ReflectMethod(RequestResample)[ScriptMethod = "Resample"];
		ReflectMethod(RequestBindRootEntity)[ScriptMethodLocked = "BindRootEntity"];
		ReflectMethod(RequestSetDebugMode)[ScriptMethodLocked = "SetDebugMode"];
	}

	return *this;
}

TShared<PhaseComponent> PhaseComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& portName) {
	CHECK_REFERENCES_NONE();

	RenderFlowComponent* ptr = renderFlowComponent.Get();
	TShared<PhaseComponent> phaseComponent = TShared<PhaseComponent>::From(allocator->New(ptr, portName));
	phaseComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return phaseComponent;
}

void PhaseComponentModule::RequestSetup(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount, uint32_t taskCount, const Float3& range, const UShort2& resolution) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(phaseComponent);
	CHECK_THREAD_IN_MODULE(phaseComponent);

	phaseComponent->Setup(engine, phaseCount, taskCount, range, resolution);
}

void PhaseComponentModule::RequestUpdate(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, const Float3& position) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(phaseComponent);
	CHECK_THREAD_IN_MODULE(phaseComponent);

	phaseComponent->Update(engine, position);
}

void PhaseComponentModule::RequestStep(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t bounceCount) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(phaseComponent);
	CHECK_THREAD_IN_MODULE(phaseComponent);

	phaseComponent->Step(engine, bounceCount);
}

void PhaseComponentModule::RequestResample(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(phaseComponent);
	CHECK_THREAD_IN_MODULE(phaseComponent);

	phaseComponent->Resample(engine, phaseCount);
}

void PhaseComponentModule::RequestBindRootEntity(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, IScript::Delegate<Entity> rootEntity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(phaseComponent);
	CHECK_DELEGATE(rootEntity);
	CHECK_THREAD_IN_MODULE(phaseComponent);

	phaseComponent->BindRootEntity(engine, rootEntity.Get());
}

void PhaseComponentModule::RequestSetDebugMode(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, const String& debugPath) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(phaseComponent);
	CHECK_THREAD_IN_MODULE(phaseComponent);

	phaseComponent->SetDebugMode(debugPath);
}
