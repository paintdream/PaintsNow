// SoundComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/StreamResource.h"

namespace PaintsNow {
	class SoundComponent : public TAllocatedTiny<SoundComponent, Component> {
	public:
		enum {
			SOUNDCOMPONENT_ONLINE = COMPONENT_CUSTOM_BEGIN,
			SOUNDCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		SoundComponent(const TShared<StreamResource>& audioResource, IScript::Request::Ref callback);
		~SoundComponent() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;

		void Step(IScript::Request& request);
		double GetDuration() const;
		void Seek(Engine& engine, double time);
		void Play(Engine& engine);
		void Pause(Engine& engine);
		void Stop(Engine& engine);
		void Rewind(Engine& engine);
		void ScriptUninitialize(IScript::Request& request) override;
		bool IsOnline() const;
		bool IsPlaying() const;

	protected:
		IAudio::Decoder* audioStream;
		IAudio::Source* audioSource;
		IAudio::Buffer* audioBuffer;
		TShared<StreamResource> audioResource;
		TWrapper<size_t> stepWrapper;
		IScript::Request::Ref callback;
	};
}

