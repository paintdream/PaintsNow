#include "StreamComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(StreamComponentModule);
StreamComponentModule::StreamComponentModule(Engine& engine) : BaseClass(engine) {}
StreamComponentModule::~StreamComponentModule() {}

TObject<IReflect>& StreamComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetLoadHandler)[ScriptMethod = "SetLoadHandler"];
		ReflectMethod(RequestSetRefreshHandler)[ScriptMethod = "SetRefreshHandler"];
		ReflectMethod(RequestSetUnloadHandler)[ScriptMethod = "SetUnloadHandler"];
	}

	return *this;
}

TShared<StreamComponent> StreamComponentModule::RequestNew(IScript::Request& request, const UShort3& dimension, uint16_t cacheCount) {
	TShared<StreamComponent> soundComponent = TShared<StreamComponent>::From(allocator->New(dimension, cacheCount));
	soundComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return soundComponent;
}

void StreamComponentModule::RequestSetLoadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> stream, IScript::Request::Ref ref) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(stream);

	stream->SetLoadHandler(request, ref);
}

void StreamComponentModule::RequestSetRefreshHandler(IScript::Request& request, IScript::Delegate<StreamComponent> stream, IScript::Request::Ref ref) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(stream);

	stream->SetRefreshHandler(request, ref);
}

void StreamComponentModule::RequestSetUnloadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> stream, IScript::Request::Ref ref) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(stream);

	stream->SetUnloadHandler(request, ref);
}
