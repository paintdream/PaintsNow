#include "TransformComponentModule.h"

using namespace PaintsNow;

CREATE_MODULE(TransformComponentModule);
TransformComponentModule::TransformComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& TransformComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];

		ReflectMethod(RequestEditorRotate)[ScriptMethodLocked = "EditorRotate"];
		ReflectMethod(RequestSetTranslation)[ScriptMethodLocked = "SetTranslation"];
		ReflectMethod(RequestGetTranslation)[ScriptMethodLocked = "GetTranslation"];
		ReflectMethod(RequestGetQuickTranslation)[ScriptMethodLocked = "GetQuickTranslation"];
		ReflectMethod(RequestSetRotation)[ScriptMethodLocked = "SetRotation"];
		ReflectMethod(RequestGetRotation)[ScriptMethodLocked = "GetRotation"];
		ReflectMethod(RequestSetScale)[ScriptMethodLocked = "SetScale"];
		ReflectMethod(RequestGetScale)[ScriptMethodLocked = "GetScale"];
		ReflectMethod(RequestGetAxises)[ScriptMethodLocked = "GetAxises"];
		ReflectMethod(RequestGetDynamic)[ScriptMethodLocked = "GetDynamic"];
		ReflectMethod(RequestSetDynamic)[ScriptMethodLocked = "SetDynamic"];
		ReflectMethod(RequestUpdateTransform)[ScriptMethodLocked = "UpdateTransform"];
	}

	return *this;
}

TShared<TransformComponent> TransformComponentModule::RequestNew(IScript::Request& request) {
	TShared<TransformComponent> transformComponent = TShared<TransformComponent>::From(allocator->New());
	transformComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return transformComponent;
}

Float3 TransformComponentModule::RequestGetQuickTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	return transformComponent->GetQuickTranslation();
}

void TransformComponentModule::RequestEditorRotate(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float2& from, Float2& to) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->EditorRotate(from, to);
}

void TransformComponentModule::RequestSetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& rotation) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->SetRotation(rotation);
}

Float3 TransformComponentModule::RequestGetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);
	return transformComponent->GetRotation();
}

void TransformComponentModule::RequestSetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& scale) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->SetScale(scale);
}

Float3 TransformComponentModule::RequestGetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	return transformComponent->GetScale();
}

void TransformComponentModule::RequestSetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& translation) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->SetTranslation(translation);
}

Float3 TransformComponentModule::RequestGetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	return transformComponent->GetTranslation();
}

void TransformComponentModule::RequestGetAxises(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	Float3 x, y, z;
	transformComponent->GetAxises(x, y, z);
	request << beginarray << x << y << z << endarray;
}

void TransformComponentModule::RequestSetDynamic(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, bool isDynamic) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->SetDynamic(isDynamic);
}

bool TransformComponentModule::RequestGetDynamic(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	return transformComponent->GetDynamic();
}

void TransformComponentModule::RequestUpdateTransform(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->UpdateTransform();
}
