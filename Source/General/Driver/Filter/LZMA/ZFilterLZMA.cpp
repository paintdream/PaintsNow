#include "ZFilterLZMA.h"
#include "../../../../Core/Template/TObject.h"
#include "Core/LzmaEnc.h"
#include "Core/LzmaDec.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cassert>

using namespace PaintsNow;


class FilterLZMAImpl : public IStreamBase {
public:
	FilterLZMAImpl(IStreamBase& streamBase);

	void Flush() override;
	bool Read(void* p, size_t& len) override;
	bool Write(const void* p, size_t& len) override;
	bool Transfer(IStreamBase& stream, size_t& len) override;
	bool WriteDummy(size_t& len) override;
	bool Seek(SEEK_OPTION option, int64_t offset) override;

	// object writing/reading routine
	virtual bool Write(IReflectObject& s, void* ptr, size_t length);
	virtual bool Read(IReflectObject& s, void* ptr, size_t length);

protected:
	IStreamBase& stream;
};


#ifdef __linux__
#define _stricmp strcasecmp
#endif

static void* MyAlloc(size_t size)
{
	return malloc(size);
}

static void MyFree(void* address)
{
	free(address);
}

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

int LzmaCompress(unsigned char *dest, size_t  *destLen, const unsigned char *src, size_t  srcLen,
					   unsigned char *outProps, size_t *outPropsSize,
					   int level, // 0 <= level <= 9, default = 5 *
					   unsigned dictSize, // use (1 << N) or (3 << N). 4 KB < dictSize <= 128 MB
					   int lc, // 0 <= lc <= 8, default = 3
					   int lp, // 0 <= lp <= 4, default = 0
					   int pb, // 0 <= pb <= 4, default = 2
					   int fb,  // 5 <= fb <= 273, default = 32
					   int numThreads //1 or 2, default = 2
					   )
{
	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	props.level = level;
	props.dictSize = dictSize;
	props.lc = lc;
	props.lp = lp;
	props.pb = pb;
	props.fb = fb;
	props.numThreads = numThreads;

	return LzmaEncode(dest, destLen, src, srcLen, &props, outProps, outPropsSize, 0, nullptr, &g_Alloc, &g_Alloc);
}

int LzmaUncompress(unsigned char *dest, size_t  *destLen, const unsigned char *src, size_t  *srcLen,
						 const unsigned char *props, size_t propsSize)
{
	ELzmaStatus status;
	return LzmaDecode(dest, destLen, src, srcLen, props, (unsigned)propsSize, LZMA_FINISH_ANY, &status, &g_Alloc);
}

void FilterLZMAImpl::Flush() {
	stream.Flush();
}

struct Props {
	unsigned char props[8];
};

bool FilterLZMAImpl::Read(void* p, size_t& len) {
	// read size first
	Props props;
	uint32_t size;
	stream >> size;
	stream >> props;

	if (size < 0x10000000) {
		bool ret = false;
		unsigned char* buffer = new unsigned char[size];
		size_t s = size;
		if (stream.Read(buffer, s)) {
			ret = ::LzmaUncompress((unsigned char*)p, &len, buffer, &s, props.props, 5) == SZ_OK;
		}

		delete[] buffer;
		return ret;
	}

	return false;
}

bool FilterLZMAImpl::Write(const void* p, size_t& len) {
	size_t size = len * 2 + 0x10000;
	unsigned char* s = new unsigned char[size];
	unsigned char props[8];
	size_t propsize;
	::LzmaCompress(s, &size, (const unsigned char*)p, len, props, &propsize, 5, 1 << 24, 3, 0, 2, 32, 1);

	size_t w = 8;
	stream.Write(props, w);
	w = sizeof(size);
	stream.Write(&size, w);
	stream.Write(s, size);
	delete[] s;

	return true;
}

bool FilterLZMAImpl::Transfer(IStreamBase& s, size_t& len) {
	assert(false); // Transfer is not allowed
	return stream.Transfer(s, len);
}

bool FilterLZMAImpl::Seek(SEEK_OPTION option, int64_t offset) {
	assert(false); // Seek is not allowed
	return stream.Seek(option, offset);
}

bool FilterLZMAImpl::WriteDummy(size_t& len) {
	assert(false); // not allowed
	return stream.WriteDummy(len);
}

FilterLZMAImpl::FilterLZMAImpl(IStreamBase& streamBase) : stream(streamBase) {}

// Reflect
class ReflectWriter : public IReflect {
public:
	ReflectWriter() : IReflect(true, false), data(nullptr) {}
	// IReflect
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (strcmp(name, "data") == 0 && typeID == UniqueType<String>::Get()) {
			data = reinterpret_cast<String*>(ptr);
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	String* data;
};

class ReflectReader : public IReflect {
public:
	ReflectReader(String& d) : IReflect(true, false), data(d) {}
	// IReflect
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (strcmp(name, "data") == 0 && typeID == UniqueType<String>::Get()) {
			std::swap(data, *reinterpret_cast<String*>(ptr));
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	String& data;
};

bool FilterLZMAImpl::Write(IReflectObject& s, void* ptr, size_t length) {
	assert(!s.IsBasicObject());
	ReflectWriter reflectWriter;
	s(reflectWriter);

	if (reflectWriter.data != nullptr) {
		uint32_t size = verify_cast<uint32_t>(reflectWriter.data->size());
		stream << size;
		size_t n = size;
		return Write(reflectWriter.data->data(), n);
	} else {
		return false;
	}
}

bool FilterLZMAImpl::Read(IReflectObject& s, void* ptr, size_t len) {
	assert(!s.IsBasicObject());
	uint32_t length = 0;
	stream >> length;
	String data;
	data.resize(length);
	size_t n = length;
	if (Read((void*)data.data(), n)) {
		ReflectReader reflectReader(data);
		s(reflectReader);

		return true;
	} else {
		return false;
	}
}

IStreamBase* ZFilterLZMA::CreateFilter(IStreamBase& streamBase) {
	return new FilterLZMAImpl(streamBase);
}