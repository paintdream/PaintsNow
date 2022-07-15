// StreamResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "../ResourceBase.h"
#include "../../../Core/Interface/IArchive.h"
#include "../../../Core/System/ShadowStream.h"

namespace PaintsNow {
	class StreamResource : public TReflected<StreamResource, DeviceResourceBase<IArchive> > {
	public:
		StreamResource(ResourceManager& manager, const String& uniqueID);

		void Refresh(IArchive& device, void* deviceContext) override;
		void Download(IArchive& device, void* deviceContext) override;
		void Upload(IArchive& device, void* deviceContext) override;
		void Attach(IArchive& device, void* deviceContext) override;
		void Detach(IArchive& device, void* deviceContext) override;

		IStreamBase& GetStream();
		bool operator << (IStreamBase& stream) override;
		bool operator >> (IStreamBase& stream) const override;
		IReflectObject* Clone() const override;

	protected:
		ShadowStream shadowStream;
	};
}

