// ZImageFreeImage.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once
#include "../../../../Core/Interface/IStreamBase.h"
#include "../../../Interface/Interfaces.h"
#include "../../../../Core/Interface/IType.h"
typedef struct FIBITMAP* PFIBITMAP;

namespace PaintsNow {
	class ZImageFreeImage final : public IImage {
	public:
		Image* Create(size_t width, size_t height, IRender::Resource::TextureDescription::Layout layout, IRender::Resource::TextureDescription::Format dataType) const override;
		IRender::Resource::TextureDescription::Layout GetLayoutType(Image* image) const override;
		IRender::Resource::TextureDescription::Format GetDataType(Image* image) const override;
		size_t GetBPP(Image* image) const override;
		size_t GetWidth(Image* image) const override;
		size_t GetHeight(Image* image) const override;
		void* GetBuffer(Image* image) const override;
		bool Load(Image* image, IStreamBase& streamBase, size_t length) const override;
		bool Save(Image* image, IStreamBase& streamBase, const String& type) const override;
		void Delete(Image* image) const override;
	};
}

