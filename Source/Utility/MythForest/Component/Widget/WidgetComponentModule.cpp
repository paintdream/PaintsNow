#include "WidgetComponentModule.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;

CREATE_MODULE(WidgetComponentModule);
WidgetComponentModule::WidgetComponentModule(Engine& engine) : BaseClass(engine) {
	batchComponentModule = (engine.GetComponentModuleFromName("BatchComponent")->QueryInterface(UniqueType<BatchComponentModule>()));
	assert(batchComponentModule != nullptr);
}

void WidgetComponentModule::Initialize() {
	SnowyStream& snowyStream = engine.snowyStream;
	widgetMesh = snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), "[Runtime]/MeshResource/StandardQuad", true, ResourceBase::RESOURCE_VIRTUAL);
	defaultWidgetMaterial = snowyStream.CreateReflectedResource(UniqueType<MaterialResource>(), "[Runtime]/MaterialResource/Widget", true, ResourceBase::RESOURCE_VIRTUAL);
	defaultWidgetTexture = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "[Runtime]/TextureResource/Black", true, ResourceBase::RESOURCE_VIRTUAL);
}

void WidgetComponentModule::Uninitialize() {
	widgetMesh = nullptr;
}

TObject<IReflect>& WidgetComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetWidgetTexture)[ScriptMethodLocked = "SetWidgetTexture"];
		ReflectMethod(RequestSetWidgetCoord)[ScriptMethodLocked = "SetWidgetCoord"];
		ReflectMethod(RequestSetWidgetRepeatMode)[ScriptMethodLocked = "SetWidgetRepeatMode"];
	}

	return *this;
}

TShared<WidgetComponent> WidgetComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<BatchComponent> batchComponent, IScript::Delegate<MaterialResource> materialResource, IScript::Delegate<TextureResource> mainTexture) {
	CHECK_REFERENCES_NONE();
	
	TShared<BatchComponent> batch;
	if (batchComponent.Get() == nullptr) {
		batch = batchComponentModule->Create(IRender::Resource::BufferDescription::UNIFORM);
	} else {
		batch = batchComponent.Get();
	}

	// must batch!
	TShared<WidgetComponent> widgetComponent = TShared<WidgetComponent>::From(allocator->New(widgetMesh, batch));
	widgetComponent->SetMainTexture(mainTexture ? mainTexture.Get() : defaultWidgetTexture);
	widgetComponent->SetMaterial(0, 0, materialResource ? TShared<MaterialResource>(materialResource.Get()) : defaultWidgetMaterial);
	widgetComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return widgetComponent;
}

void WidgetComponentModule::RequestSetWidgetTexture(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, IScript::Delegate<TextureResource> mainTexture) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(widgetComponent);
	CHECK_THREAD_IN_MODULE(widgetComponent);

	widgetComponent->SetMainTexture(mainTexture ? TShared<TextureResource>(mainTexture.Get()) : defaultWidgetTexture);
}

void WidgetComponentModule::RequestSetWidgetCoord(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, const Float4& inCoord, const Float4& outCoord) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(widgetComponent);
	CHECK_THREAD_IN_MODULE(widgetComponent);

	widgetComponent->SetCoordRect(inCoord, outCoord);
}

void WidgetComponentModule::RequestSetWidgetRepeatMode(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, bool repeatable) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(widgetComponent);
	CHECK_THREAD_IN_MODULE(widgetComponent);

	if (repeatable) {
		widgetComponent->Flag().fetch_or(repeatable ? WidgetComponent::WIDGETCOMPONENT_TEXTURE_REPEATABLE : 0);
	} else {
		widgetComponent->Flag().fetch_and(~(repeatable ? WidgetComponent::WIDGETCOMPONENT_TEXTURE_REPEATABLE : 0));
	}
}
