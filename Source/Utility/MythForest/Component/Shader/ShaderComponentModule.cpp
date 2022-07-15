#include "ShaderComponentModule.h"

using namespace PaintsNow;

CREATE_MODULE(ShaderComponentModule);
ShaderComponentModule::ShaderComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& ShaderComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetCode)[ScriptMethodLocked = "SetCode"];
		ReflectMethod(RequestSetInput)[ScriptMethodLocked = "SetInput"];
		ReflectMethod(RequestSetComplete)[ScriptMethod = "SetComplete"];
		ReflectMethod(RequestExportMaterial)[ScriptMethod = "ExportMaterial"];
	}

	return *this;
}

TShared<ShaderComponent> ShaderComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<ShaderResource> resourceTemplate, const String& name) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resourceTemplate);

	TShared<ShaderComponent> terrainComponent = TShared<ShaderComponent>::From(allocator->New(resourceTemplate.Get(), std::ref(name)));
	terrainComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return terrainComponent;
}

void ShaderComponentModule::RequestSetCode(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& text, const std::vector<std::pair<String, String> >& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderComponent);

	shaderComponent->SetCode(engine, stage, text, config);
}

void ShaderComponentModule::RequestSetInput(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderComponent);

	shaderComponent->SetInput(engine, stage, type, name, value, binding, config);
}

void ShaderComponentModule::RequestSetComplete(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderComponent);

	shaderComponent->SetComplete(engine, callback);
}

TShared<MaterialResource> ShaderComponentModule::RequestExportMaterial(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, IScript::Delegate<MaterialResource> materialResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderComponent);

	return shaderComponent->ExportMaterial(engine, materialResource.Get());
}
