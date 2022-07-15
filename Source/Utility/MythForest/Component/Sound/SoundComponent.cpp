#include "SoundComponent.h"

using namespace PaintsNow;

SoundComponent::SoundComponent(const TShared<StreamResource>& resource, IScript::Request::Ref r) : callback(r), audioStream(nullptr), audioSource(nullptr), audioBuffer(nullptr) {
	Flag().fetch_or(SOUNDCOMPONENT_ONLINE, std::memory_order_relaxed);

	audioResource.Reset(static_cast<StreamResource*>(resource->Clone()));
}

void SoundComponent::Initialize(Engine& engine, Entity* entity) {
	IAudio& audio = engine.interfaces.audio;
	audioStream = static_cast<IAudio::Decoder*>(engine.interfaces.audioFilterBase.CreateFilter(audioResource->GetStream()));
	audioBuffer = audio.CreateBuffer();
	audioSource = audio.CreateSource();
	audio.SetBufferStream(audioBuffer, *audioStream, IsOnline());
	stepWrapper = audio.SetSourceBuffer(audioSource, audioBuffer);
}

void SoundComponent::Uninitialize(Engine& engine, Entity* entity) {
	IAudio& audio = engine.interfaces.audio;
	audio.DeleteSource(audioSource);
	audio.DeleteBuffer(audioBuffer);
	audioStream->Destroy();
}

void SoundComponent::ScriptUninitialize(IScript::Request& request) {
	if (callback) {
		request.Dereference(callback);
	}

	BaseClass::ScriptUninitialize(request);
}

bool SoundComponent::IsOnline() const {
	return !!(Flag().load(std::memory_order_acquire) & SOUNDCOMPONENT_ONLINE);
}

double SoundComponent::GetDuration() const {
	return 0;
}

TObject<IReflect>& SoundComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(GetDuration);
		ReflectMethod(Seek);
		ReflectMethod(Play);
		ReflectMethod(Pause);
		ReflectMethod(Stop);
		ReflectMethod(Rewind);
		ReflectMethod(IsOnline);
		ReflectMethod(IsPlaying);
	}

	return *this;
}

bool SoundComponent::IsPlaying() const {
	return !!(Flag().load(std::memory_order_acquire) & Tiny::TINY_ACTIVATED);
}

void SoundComponent::Step(IScript::Request& request) {
	if (stepWrapper) {
		size_t t = stepWrapper();
		if (t == (size_t)-1) {
			Flag().fetch_and(~Tiny::TINY_ACTIVATED, std::memory_order_release);
		}

		if (t != 0 && callback) {
			request.DoLock();
			request.Push();
			request << (t == (size_t)-1 ? (int64_t)0 : (int64_t)t);
			request.Call(callback);
			request.Pop();
			request.UnLock();
		}
	}
}

SoundComponent::~SoundComponent() {
}

void SoundComponent::Play(Engine& engine) {
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
	engine.interfaces.audio.Play(audioSource);
}

void SoundComponent::Pause(Engine& engine) {
	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed);
	engine.interfaces.audio.Pause(audioSource);
}

void SoundComponent::Stop(Engine& engine) {
	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed);
	engine.interfaces.audio.Stop(audioSource);
}

void SoundComponent::Seek(Engine& engine, double time) {
	audioStream->Seek(IStreamBase::BEGIN, (long)(time * audioStream->GetSampleRate()));
	// engine.interfaces.audio.Seek(audioSource, IStreamBase::CUR, time);
}

void SoundComponent::Rewind(Engine& engine) {
	engine.interfaces.audio.Rewind(audioSource);
}
