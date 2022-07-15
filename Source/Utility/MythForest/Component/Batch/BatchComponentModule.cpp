#include "BatchComponentModule.h"
#include "BatchComponent.h"

using namespace PaintsNow;

CREATE_MODULE(BatchComponentModule);
BatchComponentModule::BatchComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& BatchComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestGetCaptureStatistics)[ScriptMethodLocked = "GetCaptureStatistics"];
	}

	return *this;
}

TShared<BatchComponent> BatchComponentModule::Create(IRender::Resource::BufferDescription::Usage usage) {
	TShared<BatchComponent> batchComponent = TShared<BatchComponent>::From(allocator->New(usage));
	batchComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return batchComponent;
}

TShared<BatchComponent> BatchComponentModule::RequestNew(IScript::Request& request, const String& usage) {
	CHECK_REFERENCES_NONE();
	return Create(usage == "INSTANCED" ? IRender::Resource::BufferDescription::INSTANCED : IRender::Resource::BufferDescription::UNIFORM);
}

void BatchComponentModule::RequestGetCaptureStatistics(IScript::Request& request, IScript::Delegate<BatchComponent> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(component);
}