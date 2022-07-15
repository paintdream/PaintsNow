#include "SkyComponentModule.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;

CREATE_MODULE(SkyComponentModule);
SkyComponentModule::SkyComponentModule(Engine& engine) : BaseClass(engine) {
	batchComponentModule = (engine.GetComponentModuleFromName("BatchComponent")->QueryInterface(UniqueType<BatchComponentModule>()));
	assert(batchComponentModule != nullptr);
}

void SkyComponentModule::Initialize() {
	defaultSkyMaterial = engine.snowyStream.CreateReflectedResource(UniqueType<MaterialResource>(), "[Runtime]/MaterialResource/SkyMap", true, ResourceBase::RESOURCE_VIRTUAL);
	defaultSkyMesh = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), "[Runtime]/MeshResource/StandardSphere", true, ResourceBase::RESOURCE_VIRTUAL);
}

TObject<IReflect>& SkyComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetSkyTexture)[ScriptMethodLocked = "SetSkyTexture"];
	}

	return *this;
}

TShared<SkyComponent> SkyComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource, IScript::Delegate<MaterialResource> materialResource, IScript::Delegate<BatchComponent> batch) {
	CHECK_REFERENCES_NONE();

	TShared<BatchComponent> batchComponent;
	if (batch.Get() == nullptr) {
		batchComponent = batchComponentModule->Create(IRender::Resource::BufferDescription::UNIFORM);
	} else {
		batchComponent = batch.Get();
	}

	assert(batchComponent->GetWarpIndex() == engine.GetKernel().GetCurrentWarpIndex());
	TShared<MeshResource> res = meshResource.Get();
	TShared<SkyComponent> skyComponent = TShared<SkyComponent>::From(allocator->New(meshResource ? meshResource.Get() : defaultSkyMesh, batchComponent));
	skyComponent->SetMaterial(0, 0, materialResource ? materialResource.Get() : defaultSkyMaterial);
	skyComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return skyComponent;
}

void SkyComponentModule::RequestSetSkyTexture(IScript::Request& request, IScript::Delegate<SkyComponent> skyComponent, IScript::Delegate<TextureResource> textureResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(skyComponent);

	skyComponent->SetSkyTexture(textureResource.Get());
}

