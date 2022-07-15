#include "ZFilterLZW.h"
#include "../../../../Core/Template/TObject.h"
using namespace PaintsNow;

class FilterLZWImpl : public IStreamBase {
public:
	FilterLZWImpl(IStreamBase& streamBase);
	void Clear();

	virtual void Flush();
	virtual bool Read(void* p, size_t& len);
	virtual bool Write(const void* p, size_t& len);
	virtual bool Transfer(IStreamBase& stream, size_t& len);
	virtual bool WriteDummy(size_t& len);
	virtual bool Seek(SEEK_OPTION option, int64_t offset);

	// object writing/reading routine
	virtual bool Write(IReflectObject& s, void* ptr, size_t length);
	virtual bool Read(IReflectObject& s, void* ptr, size_t length);

protected:
	int32_t ReadMap(const String& key);
	int32_t WriteMap(const String& key);
	String ReadString(int32_t v);
	void WriteString(const String& key);

protected:
	IStreamBase& stream;
	typedef std::map<String, int32_t> Map;
	std::vector<String> mapStrings;
	Map mapCodes;
};

void FilterLZWImpl::Clear() {
	mapCodes.clear();
	mapStrings.clear();
}

void FilterLZWImpl::Flush() {
	stream.Flush();
}

String FilterLZWImpl::ReadString(int32_t value) {
	if (value < 0x100) {
		return String((const char*)&value, (const char*)&value + 1);
	} else {
		return mapStrings[value - 0x100];
	}
}

void FilterLZWImpl::WriteString(const String& key) {
	mapStrings.emplace_back(key);
}

inline size_t ReadBufferHead(uint8_t& ch) {
	size_t count = 0;

	if (ch == 0xFF) {
		count = 4;
		ch = 0;
	} else if (ch >= 0xE0) {
		ch = ch & ~0xE0;
		count = 3;
	} else if (ch >= 0xC0) {
		ch = ch & ~0xC0;
		count = 2;
	} else if (ch >= 0x80) {
		ch = ch & ~0x80;
		count = 1;
	}

	return count;
}

inline int32_t ReadBuffer(int32_t res, const char* from, const char* to) {
	while (from < to) {
		res <<= 8;
		res |= (uint8_t)*from++;
	}

	return res;
}

inline bool Output(char*& buf, const String& output, long& decodeLength, size_t len) {
	if (decodeLength + output.length() > len) {
		return false;
	} else {
		memcpy(buf, output.data(), output.size());
		decodeLength += (long)output.size();
		// printf("LLLL: %d %s\n", (int)decodeLength, buf);
		buf += output.size();
		return true;
	}
}

bool FilterLZWImpl::Read(void* p, size_t& len) {
	String piece;
	long total = 0;
	size_t length = 1;
	char buffer[8];

	long decodeLength = 0;
	char* buf = (char*)p;

	String entry;
	int32_t prev = -1;

	while (decodeLength < (long)len && stream.Read(buffer, length)) {
		total++;
		uint8_t ch = *buffer;
		size_t need = ReadBufferHead(ch);
		int32_t res = ch;
		if (need != 0) {
			if (!stream.Read(buffer + 1, need)) {
				// fail!
				stream.Seek(IStreamBase::CUR, -total);
				len = total - 1;
				Clear();
				return false;
			}

			total += (long)need;
			res = ReadBuffer(res, buffer + 1, buffer + 1 + need);
		}

		// now we get res
		if (prev == -1) { // first
			prev = res;
			String output = ReadString(prev);
			if (!Output(buf, output, decodeLength, len)) {
				stream.Seek(IStreamBase::CUR, -total);
				len = (long)(total - 1 - need);
				Clear();
				return false;
			}
		} else {
			// ABA case
			if (mapStrings.size() + 0x100 <= (size_t)res) {
				entry = ReadString(prev);
				entry += entry[0];
				WriteString(entry);
				if (!Output(buf, entry, decodeLength, len)) {
					stream.Seek(IStreamBase::CUR, -total);
					len = total - 1 - need;
					Clear();
					return false;
				}
			} else {
				entry = ReadString(res);
				WriteString(ReadString(prev) + entry[0]);

				if (!Output(buf, entry, decodeLength, len)) {
					stream.Seek(IStreamBase::CUR, -total);
					len = total - 1 - need;
					Clear();
					return false;
				}
			}
			
			prev = res;
		}
	}

	return true;
}

inline void WriteBuffer(std::vector<unsigned char>& buffer, uint32_t value) {
	int count = 0;
	if (value >= 0x1F000000) {
		buffer.emplace_back(0xFF);
		count = 3;
	} else if (value >= 0x200000) {
		buffer.emplace_back(0xE0 | (uint8_t)(value >> 24));
		count = 2;
	} else if (value >= 0x4000) {
		buffer.emplace_back(0xC0 | (uint8_t)(value >> 16));
		count = 1;
	} else if (value >= 0x80) {
		buffer.emplace_back(0x80 | (uint8_t)(value >> 8));
		count = 0;
	}

	while (count >= 0) {
		buffer.emplace_back((uint8_t)(value >> (count-- * 8)));
	}
}

int32_t FilterLZWImpl::ReadMap(const String& key) {
	assert(!key.empty());
	if (key.size() == 1) {
		return key[0];
	} else {
		Map::const_iterator it = mapCodes.find(key);
		return it == mapCodes.end() ? -1 : mapCodes[key];
	}
}

int32_t FilterLZWImpl::WriteMap(const String& key) {
	assert(key.size() >= 2);
	int32_t v = (int32_t)mapCodes.size() + 0x100;
	mapCodes[key] = v;

	return v;
}

bool FilterLZWImpl::Write(const void* p, size_t& len) {
	std::vector<unsigned char> buffer;

	String s;
	String piece;
	for (const char* t = (const char*)p; t < (const char*)p + len; t++) {
		char ch = *t;
		piece = s;
		piece += ch;

		int32_t v = ReadMap(piece);
		if (v == -1) { // Not found
			WriteBuffer(buffer, ReadMap(s));
			WriteMap(piece);
			s.assign(1, ch);
		} else {
			std::swap(s, piece);
		}
	}

	if (!s.empty()) {
		WriteBuffer(buffer, ReadMap(s));
	}

	len = buffer.size();
	return buffer.empty() ? true : stream.Write(&buffer[0], len);
}

bool FilterLZWImpl::Transfer(IStreamBase& s, size_t& len) {
	assert(false); // Transfer is not allowed
	return stream.Transfer(s, len);
}

bool FilterLZWImpl::Seek(IStreamBase::SEEK_OPTION option, int64_t offset) {
	assert(false); // Seek is not allowed
	return stream.Seek(option, offset);
}

bool FilterLZWImpl::WriteDummy(size_t& len) {
	assert(false); // not allowed
	return stream.WriteDummy(len);
}

FilterLZWImpl::FilterLZWImpl(IStreamBase& streamBase) : stream(streamBase) {}

// Reflect
class ReflectWriter : public IReflect {
public:
	ReflectWriter() : IReflect(true, false), data(nullptr) {}
	// IReflect
	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		if (strcmp(name, "data") == 0 && typeID == UniqueType<String>::Get()) {
			data = reinterpret_cast<String*>(ptr);
		}
	}

	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

	String* data;
};

class ReflectReader : public IReflect {
public:
	ReflectReader(String& d) : IReflect(true, false), data(d) {}
	// IReflect
	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		if (strcmp(name, "data") == 0 && typeID == UniqueType<String>::Get()) {
			std::swap(data, *reinterpret_cast<String*>(ptr));
		}
	}

	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

	String& data;
};

bool FilterLZWImpl::Write(IReflectObject& s, void* ptr, size_t length) {
	assert(!s.IsBasicObject());
	ReflectWriter reflectWriter;
	s(reflectWriter);

	if (reflectWriter.data != nullptr) {
		uint32_t size = verify_cast<uint32_t>(reflectWriter.data->size());
		stream << size;
		size_t s = size;
		return Write(reflectWriter.data->data(), s);
	} else {
		return false;
	}
}

bool FilterLZWImpl::Read(IReflectObject& s, void* ptr, size_t len) {
	assert(!s.IsBasicObject());
	uint32_t length = 0;
	stream >> length;
	String data;
	data.resize(length);
	size_t le = length;
	if (Read((void*)data.data(), le)) {
		ReflectReader reflectReader(data);
		s(reflectReader);

		return true;
	} else {
		return false;
	}
}

IStreamBase* ZFilterLZW::CreateFilter(IStreamBase& streamBase) {
	return new FilterLZWImpl(streamBase);
}