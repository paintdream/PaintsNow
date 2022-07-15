// VolumeResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class VolumeResource : public TReflected<VolumeResource, RenderResourceBase> {
	public:
		VolumeResource(ResourceManager& manager, const String& uniqueID);
		bool operator << (IStreamBase& stream) override;
		bool operator >> (IStreamBase& stream) const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
	};
}

