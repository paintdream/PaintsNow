// TextureArrayResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "TextureResource.h"

namespace PaintsNow {
	class TextureArrayResource : public TReflected<TextureArrayResource, RenderResourceBase> {
	public:
		enum {
			TEXTUREARRAYRESOUCE_SLICE_MAPPED = RESOURCE_CUSTOM_BEGIN,
			TEXTUREARRAYRESOUCE_CUSTOM_BEGIN = RESOURCE_CUSTOM_BEGIN << 1,
		};

		TextureArrayResource(ResourceManager& manager, const String& uniqueID);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Refresh(IRender& device, void* deviceContext) override;
		void Download(IRender& device, void* deviceContext) override;
		void Upload(IRender& device, void* deviceContext) override;
		void Attach(IRender& device, void* deviceContext) override;
		void Detach(IRender& device, void* deviceContext) override;

		bool Map() override;
		bool UnMap() override;

	protected:
		std::vector<TShared<TextureResource> > slices;
		IRender::Resource* instance;
		size_t deviceMemoryUsage;
	};
}

