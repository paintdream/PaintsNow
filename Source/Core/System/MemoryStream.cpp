#include "MemoryStream.h"
#include "../Interface/IMemory.h"

using namespace PaintsNow;

MemoryStream::MemoryStream(size_t b, uint32_t align) : offset(0), totalSize(0), maxSize(b), alignment(align) {
	buffer = (uint8_t*)IMemory::AllocAligned(maxSize, alignment);
}

void MemoryStream::Realloc(size_t newSize) {
	assert(newSize > maxSize);
	size_t newMaxSize = maxSize * 2;
	while (newMaxSize < newSize) newMaxSize <<= 1;

	uint8_t* w = (uint8_t*)IMemory::AllocAligned(newMaxSize, alignment);
	memcpy(w, buffer, totalSize);
	std::swap(buffer, w);

	IMemory::FreeAligned(w, maxSize);
	maxSize = newMaxSize;
	totalSize = newSize;
}

MemoryStream::~MemoryStream() {
	IMemory::FreeAligned(buffer, maxSize);
}

bool MemoryStream::Truncate(uint64_t length) {
	size_t len = verify_cast<size_t>(length);
	if (len < maxSize) {
		uint8_t* w = (uint8_t*)IMemory::AllocAligned(len, alignment);
		memcpy(w, buffer, totalSize);
		std::swap(buffer, w);

		IMemory::FreeAligned(w, maxSize);

		maxSize = Math::Min(maxSize, len);
		totalSize = Math::Min(totalSize, len);
		offset = Math::Min(offset, len);
	}

	return true;
}

const void* MemoryStream::GetBuffer() const {
	return buffer;
}

bool MemoryStream::Read(void* p, size_t& len) {
	// printf("READ OFFSET: %d\n", offset);
	if (len + offset > totalSize) {
		len = totalSize - offset;
		return false;
	}

	memcpy(p, buffer + offset, len);
	offset += len;

	return true;
}

bool MemoryStream::CheckSize(size_t& len) {
	if (len + offset > totalSize) {
		if (len + offset <= maxSize) {
			totalSize = len + offset;
		} else  {
			Realloc(len + offset);
		}
	}

	return true;
}

IReflectObject* MemoryStream::Clone() const {
	// TODO:
	assert(false);
	return nullptr;
}

bool MemoryStream::Transfer(IStreamBase& stream, size_t& len) {
	if (!CheckSize(len)) {
		return false;
	}

	stream.Write(buffer + offset, len);
	offset += len;

	totalSize = Math::Max(totalSize, offset);
	return true;
}

void* MemoryStream::GetBuffer() {
	return buffer;
}

bool MemoryStream::Extend(size_t len) {
	if (!CheckSize(len)) {
		return false;
	}

	totalSize = Math::Max(totalSize, (size_t)offset + len);
	return true;
}

bool MemoryStream::WriteDummy(size_t& len) {
	if (!CheckSize(len)) {
		return false;
	}

	offset += len;
	totalSize = Math::Max(totalSize, (size_t)offset);
	return true;
}

bool MemoryStream::Write(const void* p, size_t& len) {
	// printf("WRITE OFFSET: %d\n", offset);
	if (!CheckSize(len)) {
		return false;
	}

	memcpy(buffer + offset, p, len);
	offset += len;
	totalSize = Math::Max(totalSize, (size_t)offset);

	return true;
}

void MemoryStream::Flush() {

}

void MemoryStream::SetEnd() {
	totalSize = offset;
}

bool MemoryStream::Seek(SEEK_OPTION option, int64_t f) {
	int64_t next;
	switch (option) {
	case IStreamBase::BEGIN:
		if (f < 0 || (size_t)f > totalSize)
			return false;

		offset = (size_t)f;
		return true;
	case IStreamBase::CUR:
		next = (int64_t)offset + f;
		if (next >= 0 && next <= (int64_t)totalSize) {
			offset = (size_t)next;
			return true;
		}
		break;
	case IStreamBase::END:
		offset = totalSize;
		return true;
	}

	return false;
}

size_t MemoryStream::GetOffset() const {
	return offset;
}

size_t MemoryStream::GetTotalLength() const {
	return totalSize;
}

size_t MemoryStream::GetMaxLength() const {
	return maxSize;
}
