#include "ZFilterBPTC.h"
#include "Core/BC7CompressorDLL.h"
#include "../../../../Core/Interface/IMemory.h"
using namespace PaintsNow;

class FilterBPTCImpl : public IStreamBase {
public:
	FilterBPTCImpl(IStreamBase& streamBase) : stream(streamBase) {}

	virtual void Flush() {}
	virtual bool Read(void* p, size_t& len) { assert(false); return false; }
	virtual bool Write(const void* p, size_t& len) {
		// extract width, height
		assert(len % sizeof(UChar4) == 0); // rgba
		uint32_t level = Math::Log2x(len / sizeof(UChar4)) >> 1;
		int width = 1 << level;
		int height = 1 << level;
		size_t newLength = width * height;
		BYTE* buffer = (BYTE*)IMemory::AllocAligned(newLength, 512);
#if defined(_MSC_VER) && (_MSC_VER <= 1200 || (!defined(_M_IX86) && !defined(_M_AMD64)))
		BC7C::CompressImageBC7(reinterpret_cast<const BYTE*>(p), buffer, width, height);
#else
		BC7C::CompressImageBC7SIMD(reinterpret_cast<const BYTE*>(p), buffer, width, height);
#endif
		bool ret = stream.Write(buffer, newLength);
		IMemory::FreeAligned(buffer, 512);
		
		return ret;
	}

	virtual bool Transfer(IStreamBase& stream, size_t& len) {
		assert(false);
		return false;
	}

	virtual bool WriteDummy(size_t& len) {
		assert(false);
		return false;
	}

	virtual bool Seek(SEEK_OPTION option, int64_t offset) {
		assert(false);
		return false;
	}

protected:
	IStreamBase& stream;
};

IStreamBase* ZFilterBPTC::CreateFilter(IStreamBase& streamBase) {
	return new FilterBPTCImpl(streamBase);
}