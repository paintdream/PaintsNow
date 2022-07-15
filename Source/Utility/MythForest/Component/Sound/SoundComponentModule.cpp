#include "SoundComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(SoundComponentModule);
SoundComponentModule::SoundComponentModule(Engine& engine) : BaseClass(engine) {}
SoundComponentModule::~SoundComponentModule() {}

TObject<IReflect>& SoundComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestGetSourceDuration)[ScriptMethodLocked = "GetSourceDuration"];
		ReflectMethod(RequestSeekSource)[ScriptMethod = "SeekSource"];
		ReflectMethod(RequestPlaySource)[ScriptMethod = "PlaySource"];
		ReflectMethod(RequestPauseSource)[ScriptMethod = "PauseSource"];
		ReflectMethod(RequestStopSource)[ScriptMethod = "StopSource"];
		ReflectMethod(RequestRewindSource)[ScriptMethod = "RewindSource"];
		ReflectMethod(RequestIsSourcePaused)[ScriptMethodLocked = "IsSourcePaused"];
	}

	return *this;
}

TShared<SoundComponent> SoundComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<StreamResource> streamResource, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(streamResource);

	TShared<SoundComponent> soundComponent = TShared<SoundComponent>::From(allocator->New(streamResource.Get(), callback));
	soundComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return soundComponent;
}

void SoundComponentModule::RequestSeekSource(IScript::Request& request, IScript::Delegate<SoundComponent> source, double time) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Seek(engine, time);
}

double SoundComponentModule::RequestGetSourceDuration(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	return source->GetDuration();
}

void SoundComponentModule::RequestPlaySource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Play(engine);
}

void SoundComponentModule::RequestPauseSource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Pause(engine);
}

void SoundComponentModule::RequestStopSource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Stop(engine);
}

void SoundComponentModule::RequestRewindSource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Rewind(engine);
}

bool SoundComponentModule::RequestIsSourcePaused(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	return !source->IsPlaying();
}
