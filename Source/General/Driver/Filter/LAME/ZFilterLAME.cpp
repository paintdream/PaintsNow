#include "../../../../Core/Template/TAlgorithm.h"
#include "ZFilterLAME.h"
#include <cassert>

extern "C" {
	#include "Core/include/lame.h"
}

using namespace PaintsNow;

class LAMEDecoder : public IAudio::Decoder {
public:
	LAMEDecoder();
	// derived from IStreamBase
	void Flush() override;
	virtual long GetRemaining() const;
	bool Read(void* p, size_t& len) override;
	bool Write(const void* p, size_t& len) override;
	bool Transfer(IStreamBase& stream, size_t& len) override;
	bool WriteDummy(size_t& len) override;
	bool Seek(IStreamBase::SEEK_OPTION option, int64_t offset) override;

	// derived from IFilterBase
	virtual void Attach(IStreamBase& inputStream);
	virtual void Detach();

	// derived from IAudio::Decoder
	// Notice that these functions are valid only after a successful call to Read().
	size_t GetSampleRate() const override;
	FORMAT GetFormat() const override;
	size_t GetSampleCount() const override;
	enum { SAMPLE_COUNT = 4096 };
	enum { BUFFER_LENGTH = 4096 };

	private:
		IStreamBase* sourceStream;
		size_t sampleRate;
		size_t sampleCount;
		size_t bitrate;
		FORMAT format;
		hip_t hip;
		short extra[2][SAMPLE_COUNT];
		int extraStart;
		int extraCount;
	};

LAMEDecoder::LAMEDecoder() : sourceStream(nullptr), sampleRate(44100), sampleCount(0), bitrate(0), format(FORMAT::STEREO16), hip(nullptr), extraStart(0), extraCount(0) { }

void LAMEDecoder::Detach() {
	if (hip != nullptr) {
		hip_decode_exit(hip);
		extraStart = extraCount = 0;
		hip = nullptr;
	}
}

void LAMEDecoder::Attach(IStreamBase& stream) {
	Detach(); // close previous buffer (if exists)
	sourceStream = &stream;
	hip = hip_decode_init();
}

void LAMEDecoder::Flush() {

}

long LAMEDecoder::GetRemaining() const {
	assert(false); // not implemented
	return 0;
}

bool LAMEDecoder::Seek(IStreamBase::SEEK_OPTION option, int64_t offset) {
	bool ret = false;
	if (offset == 0) {
		// accurate
		ret = sourceStream->Seek(option, 0);
	} else {
		// convert
		// get kbps
		if (!sourceStream->Seek(option, (long)(offset / sampleRate * bitrate / 8))) {
			if (offset > 0) {
				ret = sourceStream->Seek(IStreamBase::END, 0);
			} else {
				ret = sourceStream->Seek(IStreamBase::BEGIN, 0);
			}
		}
	}

	if (ret) {
		// reinitialize hip decoder
		hip_decode_exit(hip);
		hip = hip_decode_init();
	}

	return ret;
}

struct id3v2 {
	char header[3];
	char ver;
	char revision;
	char flag;
	char size[4];
};

bool LAMEDecoder::Read(void* p, size_t& len) {
	assert(sourceStream != nullptr);
	unsigned char buffer[BUFFER_LENGTH];

	// read mp3 data from source stream and converts it to PCM data
	// Read buffer
	bool error = false;
	size_t read = 0;
	mp3data_struct header;
	short* data = reinterpret_cast<short*>(p);

	// read extra data
	size_t length = len;
	size_t limit = Math::Min((size_t)extraCount, length / (2 * sizeof(short)));
	for (size_t k = 0; k < limit; k++) {
		for (size_t j = 0; j < 2; j++) {
			*data++ = extra[j][k + extraStart];
		}
	}

	read = limit * 2 * sizeof(short);
	extraCount -= (int)limit;

	// continue to read
	size_t size = 0;
	unsigned char* buf = nullptr;
	while (read < len) {
		assert(extraCount == 0);

		int ret = hip_decode1_headers(hip, buf, size, extra[0], extra[1], &header);
		if (ret == -1) {
			error = true;
			break;
		} else if (ret > 0 && header.header_parsed) {
			assert(ret <= SAMPLE_COUNT);
			sampleRate = header.samplerate;
			sampleCount = (size_t)header.nsamp;
			bitrate = (size_t)header.bitrate;

			// format = IAudio::Decoder::FORMAT::STEREO16;
			// data persent
			// write data
			size_t act = Math::Min((size_t)ret, (len - read) / (2 * sizeof(short)));
			for (size_t i = 0; i < act; i++) {
				for (size_t j = 0; j < 2; j++) {
					*data++ = extra[j][i];
				}
			}

			if (read + ret * 2 * sizeof(short) > len) {
				// not enough space
				extraStart = (int)act;
				extraCount = (int)(ret - act);
				read = len;
				break;
			} else {
				read += act * 2 * sizeof(short);
			}

			buf = nullptr;
			size = 0;
		} else {
			size = BUFFER_LENGTH;
			buf = buffer;

			if (!sourceStream->ReadBlock(buffer, size)) {
				error = true;
				break;
			} else {
				// test if there exists id3tag
				if (size > sizeof(id3v2)) {
					const id3v2* tag = reinterpret_cast<const id3v2*>(buffer);
					if (memcmp(tag->header, "ID3\x03", 3) == 0) {
						// get size
						size_t tagSize = tag->size[0] * 0x200000 + tag->size[1] * 0x4000 + tag->size[2] * 0x80 + tag->size[3] + 10; // ((tag->flag & (1 << 6)) ? 10 : 0);
						// printf("TAG: %X\n", tag->flag);
						// skip tagSize
						if (tagSize > size) {
							if (!sourceStream->Seek(IStreamBase::CUR, (long)(tagSize - size))) {
								error = true;
								break;
							}
							size = 0;
						} else {
							size -= tagSize;
							memmove(buffer, buffer + tagSize, size);
						}
					}
				}
			}
		}
	}

	len = read; // write actual read bytes back
	return !error;
}

size_t LAMEDecoder::GetSampleCount() const {
	return sampleCount;
}

bool LAMEDecoder::Write(const void* p, size_t& len) {
	// do not provide mp3 encode functions
	assert(false);
	return false;
}

bool LAMEDecoder::Transfer(IStreamBase& stream, size_t& len) {
	assert(false);
	return false;
}

bool LAMEDecoder::WriteDummy(size_t& len) {
	assert(false);
	return false;
}

size_t LAMEDecoder::GetSampleRate() const {
	return sampleRate;
}

IAudio::Decoder::FORMAT LAMEDecoder::GetFormat() const {
	return format;
}

IStreamBase* ZFilterLAME::CreateFilter(IStreamBase& baseStream) {
	LAMEDecoder* stream = new LAMEDecoder();
	stream->Attach(baseStream);
	return stream;
}
