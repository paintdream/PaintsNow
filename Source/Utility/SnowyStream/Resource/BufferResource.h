// BufferResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "../../../General/Misc/PassBase.h"

namespace PaintsNow {
	class BufferResource : public TReflected<BufferResource, RenderResourceBase> {
	public:
		BufferResource(ResourceManager& manager, const String& uniqueID);
		~BufferResource() override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Refresh(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		bool UnMap() override;
		size_t ReportDeviceMemoryUsage() const override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		IRender::Resource* buffer;
		IRender::Resource::BufferDescription description;
	};
}

