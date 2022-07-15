#include "ZFilterASTC.h"
#include "../../../../Core/Interface/IMemory.h"
#include "Core/astcenc.h"
using namespace PaintsNow;

class FilterASTCImpl : public IStreamBase {
public:
	FilterASTCImpl(IStreamBase& streamBase) : stream(streamBase) {}

	virtual void Flush() {}
	virtual bool Read(void* p, size_t& len) { assert(false); return false; }
	virtual bool Write(const void* p, size_t& len) {
		// extract width, height
		assert(len % sizeof(UChar4) == 0); // rgba
		uint32_t level = Math::Log2x(len / sizeof(UChar4)) >> 1;
		int width = 1 << level;
		int height = 1 << level;
		size_t newLength = width * height;
		uint8_t* buffer = (uint8_t*)IMemory::AllocAligned(newLength, 512);
		astcenc_config config;
		astcenc_config_init(ASTCENC_PRF_LDR, 4, 4, 1, ASTCENC_PRE_MEDIUM, 0, &config);
		astcenc_context* context;
		astcenc_context_alloc(&config, 1, &context);
		astcenc_image image;
		image.data = (void**)&p;
		image.dim_x = width;
		image.dim_y = height;
		image.dim_z = 1;
		image.data_type = ASTCENC_TYPE_U8;

		astcenc_swizzle swizzle;
		swizzle.r = ASTCENC_SWZ_R;
		swizzle.g = ASTCENC_SWZ_G;
		swizzle.b = ASTCENC_SWZ_B;
		swizzle.a = ASTCENC_SWZ_A;

		astcenc_compress_image(context, &image, swizzle, buffer, newLength, 0);
		bool ret = stream.Write(buffer, newLength);
		IMemory::FreeAligned(buffer, 512);
		astcenc_context_free(context);
		
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

IStreamBase* ZFilterASTC::CreateFilter(IStreamBase& streamBase) {
	return new FilterASTCImpl(streamBase);
}