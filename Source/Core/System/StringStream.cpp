#include "StringStream.h"

using namespace PaintsNow;

StringStream::StringStream(String& s, int64_t pos) : storage(s), location(pos) {}
StringStream::~StringStream() {}

bool StringStream::Read(void* p, size_t& len) {
	if (len + location <= storage.size()) {
		memcpy(p, storage.data() + location, len);
		location += len;
		return true;
	} else {
		return false;
	}
}

bool StringStream::Transfer(IStreamBase& stream, size_t& len) {
	if (len + location <= storage.size()) {
		bool ret = stream.Write(storage.data() + location, len);
		if (ret) {
			location += len;
			return true;
		}
	}

	return false;
}

bool StringStream::WriteDummy(size_t& len) {
	if (len + location <= storage.size()) {
		location += len;
		return true;
	} else {
		return false;
	}
}

bool StringStream::Write(const void* p, size_t& len) {
	if (len + location <= storage.size()) {
		memcpy(const_cast<char*>(storage.data()) + location, p, len);
		location += len;
		return true;
	} else {
		return false;
	}
}

void StringStream::Flush() {}

bool StringStream::Seek(SEEK_OPTION option, int64_t f) {
	int64_t next;
	switch (option) {
		case SEEK_OPTION::BEGIN:
			if (f < 0 || f > (int64_t)storage.size())
				return false;
			location = f;
			return true;
		case SEEK_OPTION::CUR:
			next = location + f;
			if (next >= 0 && next <= (int64_t)storage.size()) {
				location = next;
				return true;
			}
			break;
		case SEEK_OPTION::END:
			location = storage.size();
			return true;
	}

	return false;
}
