#include "ModelComponentModule.h"

using namespace PaintsNow;

CREATE_MODULE(ModelComponentModule);
ModelComponentModule::ModelComponentModule(Engine& engine) : BaseClass(engine) {
	batchComponentModule = (engine.GetComponentModuleFromName("BatchComponent")->QueryInterface(UniqueType<BatchComponentModule>()));
	assert(batchComponentModule != nullptr);
}

TObject<IReflect>& ModelComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetMaterial)[ScriptMethodLocked = "SetMaterial"];
		ReflectMethod(RequestClearMaterials)[ScriptMethodLocked = "ClearMaterials"];
		ReflectMethod(RequestUpdateMaterials)[ScriptMethodLocked = "UpdateMaterials"];
	}

	return *this;
}

TShared<ModelComponent> ModelComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource,  IScript::Delegate<BatchComponent> batch) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(meshResource);

	TShared<BatchComponent> batchComponent;
	if (batch.Get() == nullptr) {
		batchComponent = batchComponentModule->Create(IRender::Resource::BufferDescription::UNIFORM);
	} else {
		batchComponent = batch.Get();
	}

	// Not required
	// assert(batchComponent->GetWarpIndex() == engine.GetKernel().GetCurrentWarpIndex());
	TShared<MeshResource> res = meshResource.Get();
	TShared<ModelComponent> modelComponent = TShared<ModelComponent>::From(allocator->New(res, batchComponent));
	modelComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return modelComponent;
}

void ModelComponentModule::RequestSetMaterial(IScript::Request& request, IScript::Delegate<ModelComponent> modelComponent, uint16_t meshGroupIndex, uint16_t priority,  IScript::Delegate<MaterialResource> materialResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(modelComponent);
	CHECK_DELEGATE(materialResource);

	TShared<MaterialResource> mat = materialResource.Get();
	modelComponent->SetMaterial(meshGroupIndex, priority, mat);
}

void ModelComponentModule::RequestClearMaterials(IScript::Request& request, IScript::Delegate<ModelComponent> modelComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(modelComponent);

	modelComponent->ClearMaterials();
}

void ModelComponentModule::RequestUpdateMaterials(IScript::Request& request, IScript::Delegate<ModelComponent> modelComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(modelComponent);

	modelComponent->UpdateMaterials();
}
