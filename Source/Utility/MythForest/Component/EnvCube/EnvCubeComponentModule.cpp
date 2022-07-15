#include "EnvCubeComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(EnvCubeComponentModule);
EnvCubeComponentModule::EnvCubeComponentModule(Engine& engine) : BaseClass(engine) {}
EnvCubeComponentModule::~EnvCubeComponentModule() {}

TObject<IReflect>& EnvCubeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetTexture)[ScriptMethodLocked = "SetTexture"];
		ReflectMethod(RequestSetRange)[ScriptMethodLocked = "SetRange"];
		ReflectMethod(RequestSetStrength)[ScriptMethodLocked = "SetStrength"];
	}

	return *this;
}

TShared<EnvCubeComponent> EnvCubeComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<EnvCubeComponent> envCubeComponent = TShared<EnvCubeComponent>::From(allocator->New());
	envCubeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return envCubeComponent;
}

void EnvCubeComponentModule::RequestSetTexture(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, IScript::Delegate<TextureResource> texture) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(envCubeComponent);

	envCubeComponent->cubeMapTexture = texture.Get();
}

void EnvCubeComponentModule::RequestSetRange(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, Float3& range) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(envCubeComponent);

	envCubeComponent->range = range;
}

void EnvCubeComponentModule::RequestSetStrength(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, float strength) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(envCubeComponent);

	envCubeComponent->strength = strength;
}