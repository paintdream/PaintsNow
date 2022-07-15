#include "ShadowStream.h"

using namespace PaintsNow;

ShadowStream::ShadowStream() : baseStream(nullptr), length(0) {}
ShadowStream::~ShadowStream() {
	Cleanup();
}

ShadowStream& ShadowStream::operator = (const ShadowStream& rhs) {
	Cleanup();
	length = rhs.length;
	baseStream = static_cast<IStreamBase*>(baseStream->Clone());
	return *this;
}

void ShadowStream::Cleanup() {
	if (baseStream != nullptr) {
		baseStream->Destroy();
	}
}

bool ShadowStream::operator << (IStreamBase& stream) {
	Cleanup();

	stream >> length;
	baseStream = static_cast<IStreamBase*>(stream.Clone());
	assert(baseStream != nullptr);
	stream.Seek(IStreamBase::CUR, length); // skip the stream

	return true;
}

bool ShadowStream::operator >> (IStreamBase& stream) const {
	assert(!payload.Empty());
	size_t len = payload.GetSize();
	stream << (uint64_t)len;
	return stream.Write(payload.GetData(), len);
}

bool ShadowStream::Read(void* p, size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->Read(p, len);
}

bool ShadowStream::Transfer(IStreamBase& stream, size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->Transfer(stream, len);
}

bool ShadowStream::WriteDummy(size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->WriteDummy(len);
}

bool ShadowStream::Write(const void* p, size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->Write(p, len);
}

void ShadowStream::Flush() {
	assert(baseStream != nullptr);
	baseStream->Flush();
}

bool ShadowStream::Seek(SEEK_OPTION option, int64_t f) {
	assert(baseStream != nullptr);
	return baseStream->Seek(option, f);
}

Bytes& ShadowStream::GetPayload() {
	return payload;
}

const Bytes& ShadowStream::GetPayload() const {
	return payload;
}
