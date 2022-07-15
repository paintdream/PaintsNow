#include "FollowComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(FollowComponentModule);
FollowComponentModule::FollowComponentModule(Engine& engine) : BaseClass(engine) {}
FollowComponentModule::~FollowComponentModule() {}

TObject<IReflect>& FollowComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestAttach)[ScriptMethodLocked = "Attach"];
	}

	return *this;
}

TShared<FollowComponent> FollowComponentModule::RequestNew(IScript::Request& request, uint32_t bufferSize, uint32_t delayInterval) {
	TShared<FollowComponent> followComponent = TShared<FollowComponent>::From(allocator->New(bufferSize, delayInterval));
	followComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return followComponent;
}

void FollowComponentModule::RequestAttach(IScript::Request& request, IScript::Delegate<FollowComponent> followComponent, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(followComponent);
	CHECK_THREAD_IN_MODULE(followComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	followComponent->Attach(transformComponent.Get());
}


