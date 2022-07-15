#include "FieldComponentModule.h"
#include "../../Engine.h"
#include "Types/FieldSimplygon.h"
#include "Types/FieldTexture.h"

using namespace PaintsNow;

CREATE_MODULE(FieldComponentModule);
FieldComponentModule::FieldComponentModule(Engine& engine) : BaseClass(engine) {}
FieldComponentModule::~FieldComponentModule() {}

TObject<IReflect>& FieldComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestLoadSimplygon)[ScriptMethod = "LoadSimplygon"];
		ReflectMethod(RequestLoadTexture)[ScriptMethod = "LoadTexture"];
		ReflectMethod(RequestLoadMesh)[ScriptMethod = "LoadMesh"];
		ReflectMethod(RequestQueryData)[ScriptMethod = "QueryData"];
		ReflectMethod(RequestQuerySpaceEntities)[ScriptMethod = "QuerySpaceEntities"];
		ReflectMethod(RequestPostSpaceEvent)[ScriptMethod = "PostSpaceEvent"];
	}

	return *this;
}

TShared<FieldComponent> FieldComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<FieldComponent> fieldComponent = TShared<FieldComponent>::From(allocator->New());
	fieldComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return fieldComponent;
}

String FieldComponentModule::RequestQueryData(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const Float3& position) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

	Bytes result = (*fieldComponent.Get())[position];
	return String((char*)result.GetData(), result.GetSize());
}

void FieldComponentModule::RequestLoadSimplygon(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const String& shapeType, const Float3Pair& range) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

	FieldSimplygon::SIMPOLYGON_TYPE type = FieldSimplygon::BOUNDING_BOX;

	if (shapeType == "Box") {
		type = FieldSimplygon::BOUNDING_BOX;
	} else if (shapeType == "Sphere") {
		type = FieldSimplygon::BOUNDING_SPHERE;
	} else if (shapeType == "Cylinder") {
		type = FieldSimplygon::BOUNDING_CYLINDER;
	}

	TShared<FieldSimplygon> instance = TShared<FieldSimplygon>(new FieldSimplygon(type, range));

	fieldComponent->SetField(instance());
}

void FieldComponentModule::RequestLoadTexture(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<TextureResource> textureResource, const Float3Pair& range) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(textureResource);

	TShared<FieldTexture> instance = TShared<FieldTexture>(new FieldTexture(textureResource.Get(), range));

	fieldComponent->SetField(instance());
}

void FieldComponentModule::RequestLoadMesh(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<MeshResource> meshResource, const Float3Pair& range) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(meshResource);
	
}

std::vector<TShared<Entity> > FieldComponentModule::RequestQuerySpaceEntities(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<SpaceComponent> spaceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);

	std::vector<TShared<Entity> > entities;
	fieldComponent->QueryEntities(spaceComponent.Get(), entities);
	return entities;
}

void FieldComponentModule::RequestPostSpaceEvent(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<SpaceComponent> spaceComponent, const String& type, IScript::Delegate<SharedTiny> sender,  IScript::Request::Ref param) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);

	TShared<TSharedTinyWrapper<IScript::Request::Ref> > wrapper;
	if (param) {
		wrapper.Reset(new TSharedTinyWrapper<IScript::Request::Ref>(param));
	}

	// now we only support post custom events and update event.
	Event event(engine, type == "Update" ? Event::EVENT_UPDATE : Event::EVENT_CUSTOM, sender.Get(), wrapper());

	if (param) {
		request.DoLock();
		request.Dereference(param);
		request.UnLock();
	}
}
