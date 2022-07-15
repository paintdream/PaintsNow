#pragma once
#include "../../../Interface/IAudio.h"
#include <cstdlib>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#include <AL/alc.h>
#include <AL/al.h>
#else
#define AL_LIBTYPE_STATIC 1
#include "Core/include/AL/alc.h"
#include "Core/include/AL/al.h"
#endif

namespace PaintsNow {
	class ZAudioOpenAL final : public IAudio {
	public:
		ZAudioOpenAL();
		~ZAudioOpenAL() override;
		IAudio::Buffer* CreateBuffer() override;
		void SetBufferStream(Buffer* buffer, IAudio::Decoder& stream, bool online) override;
		virtual void SetBufferData(Buffer* buffer, const void* data, size_t length, Decoder::FORMAT dataType, size_t sampleRate);
		void DeleteBuffer(Buffer* buffer) override;
		IAudio::Source* CreateSource() override;
		void DeleteSource(Source* sourceHandle) override;
		void SetSourcePosition(Source* sourceHandle, const Float3& position) override;
		void SetSourceVolume(Source* sourceHandle, float volume) override;
		TWrapper<size_t> SetSourceBuffer(Source* sourceHandle, const Buffer* buffer) override;
		void SetListenerPosition(const Float3& position) override;
		void Play(Source* sourceHandle) override;
		void Pause(Source* sourceHandle) override;
		void Rewind(Source* sourceHandle) override;
		void Stop(Source* sourceHandle) override;
		void ResetBuffer(Buffer* buffer);
		// virtual void Seek(Source* sourceHandle, IStreamBase::SEEK_OPTION option, int64_t offset);

	private:
		void SwitchBufferType(IAudio::Buffer* buffer, bool online);
		ALCdevice* device;
		ALCcontext* context;
	};
}

