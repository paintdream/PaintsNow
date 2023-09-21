// TextureResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"

namespace PaintsNow {
	class TextureResource : public TReflected<TextureResource, RenderResourceBase> {
	public:
		TextureResource(ResourceManager& manager, const String& uniqueID);
		enum {
			TEXTURERESOURCE_FORCERAWDATA = ResourceBase::RESOURCE_CUSTOM_BEGIN,
			TEXTURERESOURCE_CUSTOM_BEGIN = ResourceBase::RESOURCE_CUSTOM_BEGIN << 1,
		};

		IStreamBase* OpenArchive(IArchive& archive, const String& extension, bool write, uint64_t& length) override;
		size_t ReportDeviceMemoryUsage() const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		bool Compress(const String& compressType, bool refreshRuntime) override;
		bool LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) override;
		bool UnMap() override;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		IRender::Resource* GetRenderResource() const;
		IRender::Resource::TextureDescription description;

	private:
		IRender::Resource* instance;
		size_t deviceMemoryUsage;
	};
}

