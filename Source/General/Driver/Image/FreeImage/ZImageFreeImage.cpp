#include "ZImageFreeImage.h"

#define FREEIMAGE_LIB

#include "Core/Source/FreeImage.h"

#include <cassert>
#include <algorithm>

using namespace PaintsNow;

class InitFreeImage {
public:
	InitFreeImage() {
		FreeImage_Initialise(true);
	}

	~InitFreeImage() {
		FreeImage_DeInitialise();
	}
};

class FreeImageImpl : public IImage::Image {
public:
	FreeImageImpl(IRender::Resource::TextureDescription::Layout layout = IRender::Resource::TextureDescription::RGBA, IRender::Resource::TextureDescription::Format dt = IRender::Resource::TextureDescription::UNSIGNED_BYTE);
	~FreeImageImpl();

	bool New(size_t width, size_t height, IRender::Resource::TextureDescription::Layout layout, IRender::Resource::TextureDescription::Format dataType);
	IRender::Resource::TextureDescription::Layout GetLayoutType() const;
	IRender::Resource::TextureDescription::Format GetDataType() const;
	bool Load(IStreamBase& streamBase, size_t length);
	bool Save(IStreamBase& streamBase, const String& type) const;
	size_t GetBPP() const;
	bool IsValid() const;
	void Inverse();
	size_t GetWidth() const;
	size_t GetHeight() const;
	void* GetBuffer();

	const Float4 Get(size_t i, size_t j) const;
	void Set(size_t i, size_t j, const Float4& color);

private:
	PFIBITMAP bitmap;
	size_t width, height;
	IRender::Resource::TextureDescription::Layout layout;
	IRender::Resource::TextureDescription::Format dataType;
	int imageFormat;
};

FreeImageImpl::FreeImageImpl(IRender::Resource::TextureDescription::Layout l, IRender::Resource::TextureDescription::Format dt) : bitmap(nullptr), layout(l), dataType(dt), imageFormat(FIF_UNKNOWN) {
	static InitFreeImage initFreeImage;
	assert(dataType == IRender::Resource::TextureDescription::UNSIGNED_BYTE);
}

FreeImageImpl::~FreeImageImpl() {
	if (bitmap != nullptr) {
		FreeImage_Unload(bitmap);
	}
}

size_t FreeImageImpl::GetBPP() const {
	return IImage::GetPixelBitDepth(dataType, layout);
}

IRender::Resource::TextureDescription::Layout FreeImageImpl::GetLayoutType() const {
	return layout;
}

IRender::Resource::TextureDescription::Format FreeImageImpl::GetDataType() const {
	return dataType;
}

bool FreeImageImpl::New(size_t w, size_t h, IRender::Resource::TextureDescription::Layout lo, IRender::Resource::TextureDescription::Format dt) {
	assert(bitmap == nullptr);
	layout = lo;
	dataType = dt;

	assert(dataType == IRender::Resource::TextureDescription::UNSIGNED_BYTE); // only ub supported
	bitmap = FreeImage_Allocate((int)w, (int)h, (int)GetBPP());
	if (bitmap != nullptr) {
		width = w;
		height = h;
		return true;
	} else {
		return false;
	}
}

bool FreeImageImpl::Load(IStreamBase& stream, size_t length) {
	if (bitmap != nullptr) {
		FreeImage_Unload(bitmap);
	}

	BYTE* buffer = new BYTE[length];
	if (!stream.ReadBlock(buffer, length)) {
		delete[] buffer;
		return false;
	}

	FIMEMORY* mem = FreeImage_OpenMemory(buffer, (unsigned long)length);
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem, 0);
	if (fif != FIF_UNKNOWN) {
		imageFormat = fif;
		bitmap = FreeImage_LoadFromMemory(fif, mem, 0);
		if (bitmap != nullptr) {
			width = FreeImage_GetWidth(bitmap);
			height = FreeImage_GetHeight(bitmap);
			// convert bpp
			/*
			if ((int)FreeImage_GetBPP(bitmap) != GetBPP()) {
				FIBITMAP* w = nullptr;
				if (layout == IRender::Resource::TextureDescription::RGBA)
					w = FreeImage_ConvertTo32Bits(bitmap);
				else
					w = FreeImage_ConvertTo24Bits(bitmap);
				FreeImage_Unload(bitmap);
				bitmap = w;
			}*/
			int bpp = FreeImage_GetBPP(bitmap);
			int channelSize = sizeof(unsigned char);
			switch (FreeImage_GetImageType(bitmap)) {
				case FIT_BITMAP:
					dataType = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
					channelSize = sizeof(unsigned char);
					break;
				case FIT_INT16:
					dataType = IRender::Resource::TextureDescription::UNSIGNED_SHORT;
					channelSize = sizeof(unsigned short);
					break;
					/*
				case FIT_UINT32:
					dataType = IRender::UNSIGNED_INT;
					channelSize = sizeof(unsigned int);
					break;*/
				case FIT_FLOAT:
					dataType = IRender::Resource::TextureDescription::FLOAT;
					channelSize = sizeof(float);
					break;
				default:
					FreeImage_Unload(bitmap); // not supported
					bitmap = nullptr;
					break;
			}

			if (bitmap != nullptr) {
				switch (bpp / channelSize / 8) {
					case 1:
						layout = IRender::Resource::TextureDescription::R;
						break;
					case 2:
						layout = IRender::Resource::TextureDescription::RG;
						break;
					case 3:
						layout = IRender::Resource::TextureDescription::RGB;
						break;
					case 4:
						layout = IRender::Resource::TextureDescription::RGBA;
						break;
					default:
						FreeImage_Unload(bitmap); // not supported
						bitmap = nullptr;
						break;
				}
			}
		}
	}

	FreeImage_CloseMemory(mem);
	delete[] buffer;
	return bitmap != nullptr;
}

bool FreeImageImpl::Save(IStreamBase& stream, const String& type) const {
	size_t length;
	if (bitmap == nullptr)
		return false;

	// get type
	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(type.c_str());
	if (fif == FIF_UNKNOWN)
		return false;

	FIMEMORY* mem = FreeImage_OpenMemory(0, 0);
	if (mem == nullptr)
		return false;

	FreeImage_SaveToMemory(fif, bitmap, mem, 0);
	BYTE* data = nullptr;
	DWORD fileSize = 0;
	bool ret = false;

	if (FreeImage_AcquireMemory(mem, &data, &fileSize)) {
		length = fileSize;
		ret = stream.Write(data, length);
	}

	FreeImage_CloseMemory(mem);
	return ret;
}

size_t FreeImageImpl::GetWidth() const {
	assert(bitmap != nullptr);
	return width;
}

size_t FreeImageImpl::GetHeight() const {
	assert(bitmap != nullptr);
	return height;
}

void* FreeImageImpl::GetBuffer() {
	assert(bitmap != nullptr);
	return FreeImage_GetBits(bitmap);
}

bool FreeImageImpl::IsValid() const {
	return bitmap != nullptr;
}

const Float4 FreeImageImpl::Get(size_t i, size_t j) const {
	int step = layout == IRender::Resource::TextureDescription::RGB ? 3 : 4;
	const unsigned char* p = (const unsigned char*)FreeImage_GetBits(bitmap);
	const unsigned char* s = p + step * (i * width + j);
	return Float4((float)s[0] / 255.0f, (float)s[1] / 255.0f, (float)s[2] / 255.0f, layout == IRender::Resource::TextureDescription::RGB ? 1 : (float)s[3] / 255.0f);
}

void FreeImageImpl::Set(size_t i, size_t j, const Float4& color) {
	int step = layout == IRender::Resource::TextureDescription::RGB ? 3 : 4;
	unsigned char* p = (unsigned char*)GetBuffer();
	unsigned char* s = p + step * (i * width + j);
	s[0] = (unsigned char)(color.r() > 1 ? 255.0f : color.r() * 255.0f);
	s[1] = (unsigned char)(color.g() > 1 ? 255.0f : color.g() * 255.0f);
	s[2] = (unsigned char)(color.b() > 1 ? 255.0f : color.b() * 255.0f);
	if (layout == IRender::Resource::TextureDescription::RGB) {
		s[3] = (unsigned char)(color.a() > 1 ? 255.0f : color.a() * 255.0f);
	}
}

void FreeImageImpl::Inverse() {
	size_t step = layout == IRender::Resource::TextureDescription::RGB ? 3 : 4;
	unsigned char* p = (unsigned char*)GetBuffer();
	for (size_t i = 0; i < width * height * step; i++) {
		p[i] = 255 - p[i];
	}
}

IImage::Image* ZImageFreeImage::Create(size_t width, size_t height, IRender::Resource::TextureDescription::Layout layout, IRender::Resource::TextureDescription::Format dataType) const {
	FreeImageImpl* impl = new FreeImageImpl();
	impl->New(width, height, layout, dataType);
	return impl;
}

IRender::Resource::TextureDescription::Layout ZImageFreeImage::GetLayoutType(IImage::Image* image) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->GetLayoutType();
}

IRender::Resource::TextureDescription::Format ZImageFreeImage::GetDataType(Image* image) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->GetDataType();
}

size_t ZImageFreeImage::GetBPP(Image* image) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->GetBPP();
}

size_t ZImageFreeImage::GetWidth(Image* image) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->GetWidth();
}

size_t ZImageFreeImage::GetHeight(Image* image) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->GetHeight();
}

void* ZImageFreeImage::GetBuffer(Image* image) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->GetBuffer();
}

bool ZImageFreeImage::Load(Image* image, IStreamBase& streamBase, size_t length) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->Load(streamBase, length);
}

bool ZImageFreeImage::Save(Image* image, IStreamBase& streamBase, const String& type) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	return impl->Save(streamBase, type);
}

void ZImageFreeImage::Delete(Image* image) const {
	FreeImageImpl* impl = static_cast<FreeImageImpl*>(image);
	delete impl;
}
