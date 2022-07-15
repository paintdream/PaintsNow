#include "FormComponentModule.h"
#include "FormComponent.h"

using namespace PaintsNow;

CREATE_MODULE(FormComponentModule);
FormComponentModule::FormComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& FormComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestGetData)[ScriptMethodLocked = "GetData"];
		ReflectMethod(RequestGetName)[ScriptMethodLocked = "GetName"];
		ReflectMethod(RequestResize)[ScriptMethodLocked = "Resize"];
		ReflectMethod(RequestSetData)[ScriptMethodLocked = "SetData"];
	}

	return *this;
}

TShared<FormComponent> FormComponentModule::RequestNew(IScript::Request& request, String& name) {
	CHECK_REFERENCES_NONE();

	TShared<FormComponent> formComponent = TShared<FormComponent>::From(allocator->New(std::move(name)));
	formComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return formComponent;
}

void FormComponentModule::RequestResize(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	if (index >= 0) {
		formComponent->GetValues().resize((size_t)index);
	}
}

void FormComponentModule::RequestSetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index, String& data) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	if (index >= 0 && index < (int64_t)formComponent->GetValues().size()) {
		std::swap(formComponent->GetValues()[(size_t)index], data);
	}
}

const String& FormComponentModule::RequestGetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	if (index >= 0 && index < (int32_t)formComponent->GetValues().size()) {
		const String& v = formComponent->GetValues()[(size_t)index];
		return v;
	} else {
		static String emptyString;
		return emptyString;
	}
}

const String& FormComponentModule::RequestGetName(IScript::Request& request, IScript::Delegate<FormComponent> formComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	return formComponent->GetName();
}