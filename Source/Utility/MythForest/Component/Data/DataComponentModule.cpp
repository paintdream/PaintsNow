#include "DataComponentModule.h"
#include "DataComponent.h"

using namespace PaintsNow;

CREATE_MODULE(DataComponentModule);
DataComponentModule::DataComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& DataComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestGetProperty)[ScriptMethodLocked = "GetProperty"];
		ReflectMethod(RequestSetProperty)[ScriptMethod = "SetProperty"];
		ReflectMethod(RequestGetPropertyData)[ScriptMethodLocked = "GetPropertyData"];
		ReflectMethod(RequestSetPropertyData)[ScriptMethodLocked = "SetPropertyData"];
	}

	return *this;
}

TShared<DataComponent> DataComponentModule::RequestNew(IScript::Request& request, size_t maxCount) {
	CHECK_REFERENCES_NONE();

	TShared<DataComponent> dataComponent = TShared<DataComponent>::From(allocator->New(maxCount));
	dataComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return dataComponent;
}

size_t DataComponentModule::RequestSetProperty(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, const String& name, size_t sizeInBytes) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(dataComponent);
	CHECK_THREAD_IN_MODULE(dataComponent);

	return dataComponent->SetProperty(name, sizeInBytes);
}

void DataComponentModule::RequestGetProperty(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, const String& name) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(dataComponent);
	CHECK_THREAD_IN_MODULE(dataComponent);

	size_t index = dataComponent->GetProperty(name);
	if (index != ~(size_t)0) {
		request << index;
	}
}

void DataComponentModule::RequestSetPropertyData(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, size_t objectIndex, size_t propertyIndex, const StringView& data) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(dataComponent);
	CHECK_THREAD_IN_MODULE(dataComponent);

	uint8_t* ptr = dataComponent->GetPropertyData(objectIndex, propertyIndex);
	size_t length = dataComponent->GetPropertySize(propertyIndex);

	memcpy(ptr, data.data(), Math::Min(length, data.length()));
}

String DataComponentModule::RequestGetPropertyData(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, size_t objectIndex, size_t propertyIndex) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(dataComponent);
	CHECK_THREAD_IN_MODULE(dataComponent);

	const uint8_t* ptr = dataComponent->GetPropertyData(objectIndex, propertyIndex);
	size_t length = dataComponent->GetPropertySize(propertyIndex);

	return String(reinterpret_cast<const char*>(ptr), length);
}
