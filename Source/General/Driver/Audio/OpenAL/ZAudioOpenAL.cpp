#include "ZAudioOpenAL.h"

using namespace PaintsNow;

ZAudioOpenAL::ZAudioOpenAL() : device(nullptr), context(nullptr) {
	device = alcOpenDevice(nullptr);
	if (device != nullptr) {
		context = alcCreateContext(device, nullptr);
		if (context != nullptr) {
			alcMakeContextCurrent(context);
			alcProcessContext(context);
		}
	}
}

ZAudioOpenAL::~ZAudioOpenAL() {
	if (context != nullptr) {
		alcSuspendContext(context);
		alcDestroyContext(context);
	}

	alcMakeContextCurrent(nullptr);

	if (device != nullptr) {
		alcCloseDevice(device);
	}
}

static int ConvertFormat(IAudio::Decoder::FORMAT inputFormat) {
	int format = 0;
	switch (inputFormat) {
		case IAudio::Decoder::FORMAT::MONO8:
			format = AL_FORMAT_MONO8;
			break;
		case IAudio::Decoder::FORMAT::MONO16:
			format = AL_FORMAT_MONO16;
			break;
		case IAudio::Decoder::FORMAT::STEREO8:
			format = AL_FORMAT_STEREO8;
			break;
		case IAudio::Decoder::FORMAT::STEREO16:
			format = AL_FORMAT_STEREO16;
			break;
	}

	return format;
}

template <size_t size>
struct SizedBlock {
	operator char* () {
		return data;
	}
	char data[size];
};

typedef SizedBlock<44100 * 2 * sizeof(short) / 4> Block;

class BufferImpl : public IAudio::Buffer {
public:
	BufferImpl() : isRequireBuffer(false) {}
	bool isQueuedBuffer;
	bool isRequireBuffer;
	ALuint lastProcessed;

	union {
		struct {
			ALuint bufferID;
		};
		struct {
			ALuint bufferIDs[2];
			IAudio::Decoder* stream;
		};
	};
};

class SourceImpl : public IAudio::Source {
public:
	size_t Notify() {
		assert(buffer != nullptr);
		// if (alcGetCurrentContext() == nullptr)
		// alcMakeContextCurrent(context);
		int value;
		alGetSourcei(sourceID, AL_SOURCE_STATE, &value);
		if (value == AL_STOPPED) {
			alSourcePlay(sourceID);
			//	printf("Source state: %d\n", value);
		}

		ALuint processed;
		alSourceUnqueueBuffers(sourceID, 1, &processed);
		size_t t = buffer->stream->GetSampleRate();
		switch (buffer->stream->GetFormat()) {
			case IAudio::Decoder::FORMAT::MONO8:
				t *= sizeof(char);
				break;
			case IAudio::Decoder::FORMAT::MONO16:
				t *= sizeof(short);
				break;
			case IAudio::Decoder::FORMAT::STEREO8:
				t *= sizeof(char) * 2;
				break;
			case IAudio::Decoder::FORMAT::STEREO16:
				t *= sizeof(short) * 2;
				break;
		}

		if (buffer->isRequireBuffer) {
			processed = buffer->lastProcessed;
		} else {
			buffer->lastProcessed = processed;
		}

		ALuint error;
		if ((error = alGetError()) == AL_NO_ERROR || buffer->isRequireBuffer) {
			if (buffer->bufferIDs[0] != processed) {
				std::swap(buffer->bufferIDs[0], buffer->bufferIDs[1]);
			}

			size_t length;
			Block current;
			// read new buffer
			buffer->isRequireBuffer = false;

			length = sizeof(current);
			if (buffer->stream->ReadBlock(&current, length)) {
				alBufferData(processed, ConvertFormat(buffer->stream->GetFormat()), current, (ALsizei)length, (ALsizei)buffer->stream->GetSampleRate());
				alSourceQueueBuffers(sourceID, 1, &processed);
				std::swap(buffer->bufferIDs[0], buffer->bufferIDs[1]);

				// return duration processed
				return sizeof(Block) * 1000 / t;
			} else {
				// buffer->isRequireBuffer = true;
				// std::swap(buffer->bufferIDs[0], buffer->bufferIDs[1]);
				return (size_t)-1; // finished
			}
		} else {
			buffer->isRequireBuffer = false;
			return 0;
		}
	}

	ALuint sourceID;
	BufferImpl* buffer;
	ALCcontext* context;
};

IAudio::Buffer* ZAudioOpenAL::CreateBuffer() {
	BufferImpl* buffer = new BufferImpl();
	buffer->isQueuedBuffer = false;
	alGenBuffers(1, &buffer->bufferID);
	return buffer;
}

void ZAudioOpenAL::SwitchBufferType(IAudio::Buffer* buf, bool online) {
	BufferImpl* buffer = static_cast<BufferImpl*>(buf);
	if (buffer->isQueuedBuffer && !online) {
		alDeleteBuffers(1, &buffer->bufferIDs[1]);
		buffer->isQueuedBuffer = false;
	} else if (!buffer->isQueuedBuffer && online) {
		alGenBuffers(1, &buffer->bufferIDs[1]);
		buffer->isQueuedBuffer = true;
	}
}

/*
void ZAudioOpenAL::Seek(SourceImpl* sourceHandle, IStreamBase::SEEK_OPTION option, int64_t offset) {
	assert(sourceHandle->buffer->isQueuedBuffer);
	sourceHandle->buffer->stream->Seek(option, offset);
}*/

void ZAudioOpenAL::SetBufferStream(IAudio::Buffer* buf, IAudio::Decoder& stream, bool online) {
	BufferImpl* buffer = static_cast<BufferImpl*>(buf);
	SwitchBufferType(buffer, online);
	if (!online) {
		// change buffer type
		size_t length;
		size_t validLength = 0;
		Block current;
		std::vector<Block> total;

		while (true) {
			length = sizeof(current);
			if (!stream.ReadBlock(&current, length)) {
				break;
			}

			total.emplace_back(current);
			validLength += length;
			length = sizeof(current);
		}

		if (!total.empty()) {
			SetBufferData(buffer, &total[0], validLength, stream.GetFormat(), stream.GetSampleRate());
		}
	} else {
		// prepare two init buffers
		size_t length;
		Block current;

		for (size_t i = 0; i < 2; i++) {
			length = sizeof(current);
			if (!stream.ReadBlock(&current, length)) {
				break;
			}

			alBufferData(buffer->bufferIDs[i], ConvertFormat(stream.GetFormat()), current, (ALsizei)length, (ALsizei)stream.GetSampleRate());
		}

		buffer->stream = &stream;
	}
}

void ZAudioOpenAL::SetBufferData(IAudio::Buffer* buf, const void* data, size_t length, Decoder::FORMAT inputFormat, size_t sampleRate) {
	BufferImpl* buffer = static_cast<BufferImpl*>(buf);
	alBufferData(buffer->bufferID, ConvertFormat(inputFormat), data, (ALsizei)length, (ALsizei)sampleRate);
}

void ZAudioOpenAL::DeleteBuffer(IAudio::Buffer* buf) {
	BufferImpl* buffer = static_cast<BufferImpl*>(buf);
	alDeleteBuffers(1, &buffer->bufferID);
	if (buffer->isQueuedBuffer)
		alDeleteBuffers(1, &buffer->bufferIDs[1]);
	delete buffer;
}

IAudio::Source* ZAudioOpenAL::CreateSource() {
	SourceImpl* source = new SourceImpl();
	// int err = alGetError();
	ALuint sources[10];
	alGenSources(1, sources);
	alGenSources(1, &source->sourceID);
	source->context = context;
	return source;
}

void ZAudioOpenAL::DeleteSource(Source* source) {
	SourceImpl* sourceHandle = static_cast<SourceImpl*>(source);
	alDeleteSources(1, &sourceHandle->sourceID);
	delete sourceHandle;
}

void ZAudioOpenAL::SetSourcePosition(Source* source, const Float3& position) {
	SourceImpl* sourceHandle = static_cast<SourceImpl*>(source);
	alSource3f(sourceHandle->sourceID, AL_POSITION, position.x(), position.y(), position.z());
}

TWrapper<size_t> ZAudioOpenAL::SetSourceBuffer(Source* source, const Buffer* buf) {
	const BufferImpl* buffer = static_cast<const BufferImpl*>(buf);
	SourceImpl* sourceHandle = static_cast<SourceImpl*>(source);
	sourceHandle->buffer = const_cast<BufferImpl*>(buffer);
	if (buffer->isQueuedBuffer) {
		alSourceQueueBuffers(sourceHandle->sourceID, 2, buffer->bufferIDs);
		return Wrap(sourceHandle, &SourceImpl::Notify);
	} else {
		alSourcei(sourceHandle->sourceID, AL_BUFFER, buffer->bufferID);
		return TWrapper<size_t>();
	}
}

void ZAudioOpenAL::SetListenerPosition(const Float3& position) {
	alListener3f(AL_POSITION, position.x(), position.y(), position.z());
}

void ZAudioOpenAL::SetSourceVolume(Source* sourceHandle, float volume) {
	assert(false); // not implemented
}

void ZAudioOpenAL::Play(Source* source) {
	SourceImpl* sourceHandle = static_cast<SourceImpl*>(source);
	alSourcePlay(sourceHandle->sourceID);
}
void ZAudioOpenAL::Pause(Source* source) {
	SourceImpl* sourceHandle = static_cast<SourceImpl*>(source);
	alSourcePause(sourceHandle->sourceID);
}
void ZAudioOpenAL::Rewind(Source* source) {
	SourceImpl* sourceHandle = static_cast<SourceImpl*>(source);
	BufferImpl* buffer = sourceHandle->buffer;
	if (buffer->isQueuedBuffer) {
		assert(buffer->stream);
		// reset
		buffer->stream->Seek(IStreamBase::BEGIN, 0);
	}

	alSourceRewind(sourceHandle->sourceID);
}

void ZAudioOpenAL::ResetBuffer(IAudio::Buffer* buf) {
	BufferImpl* buffer = static_cast<BufferImpl*>(buf);
	alDeleteBuffers(1, &buffer->bufferID);
	alGenBuffers(1, &buffer->bufferID);
	if (buffer->isQueuedBuffer) {
		alDeleteBuffers(1, &buffer->bufferIDs[1]);
		alGenBuffers(1, &buffer->bufferIDs[1]);
	}
}

void ZAudioOpenAL::Stop(Source* source) {
	SourceImpl* sourceHandle = static_cast<SourceImpl*>(source);
	// reset buffer
	alSourceStop(sourceHandle->sourceID);
	Rewind(sourceHandle);
	BufferImpl* buffer = sourceHandle->buffer;
	if (buffer->isQueuedBuffer) {
		IAudio::Decoder* decoder = buffer->stream;
		ResetBuffer(buffer);
		// buffer->stream = nullptr;
		// DeleteBuffer(buffer);
		// buffer = CreateBuffer();
		SetBufferStream(buffer, *decoder, true);
		sourceHandle->buffer = buffer;
	}
}
