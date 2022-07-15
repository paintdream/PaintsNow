// IImage.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "IRender.h"
#include "../../Core/Interface/IDevice.h"
#include <string>

namespace PaintsNow {
	class IArchive;
	class pure_interface IImage : public IDevice {
	public:
		~IImage() override;
		class Image {};
		static uint32_t GetPixelBitDepth(IRender::Resource::TextureDescription::Format dataType, IRender::Resource::TextureDescription::Layout layout);
		virtual Image* Create(size_t width, size_t height, IRender::Resource::TextureDescription::Layout layout, IRender::Resource::TextureDescription::Format dataType) const = 0;
		virtual IRender::Resource::TextureDescription::Layout GetLayoutType(Image* image) const = 0;
		virtual IRender::Resource::TextureDescription::Format GetDataType(Image* image) const = 0;
		virtual size_t GetBPP(Image* image) const = 0;
		virtual size_t GetWidth(Image* image) const = 0;
		virtual size_t GetHeight(Image* image) const = 0;
		virtual void* GetBuffer(Image* image) const = 0;
		virtual bool Load(Image* image, IStreamBase& streamBase, size_t length) const = 0;
		virtual bool Save(Image* image, IStreamBase& streamBase, const String& type) const = 0;
		virtual void Delete(Image* image) const = 0;
	};
}

