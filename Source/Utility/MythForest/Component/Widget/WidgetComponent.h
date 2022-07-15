// WidgetComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Model/ModelComponent.h"

namespace PaintsNow {
	class WidgetComponent : public TAllocatedTiny<WidgetComponent, ModelComponent> {
	public:
		enum {
			WIDGETCOMPONENT_TEXTURE_REPEATABLE = MODELCOMPONENT_CUSTOM_BEGIN,
			WIDGETCOMPONENT_CUSTOM_BEGIN = MODELCOMPONENT_CUSTOM_BEGIN << 1
		};

		WidgetComponent(const TShared<MeshResource>& meshResource, const TShared<BatchComponent>& batchUniforms);

		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;
		void SetCoordRect(const Float4& texCoordBegin, const Float4& texCoordEnd);
		void SetMainTexture(const TShared<TextureResource>& mainTexture);

	protected:
		// Custom data
		Float4 texCoordBegin;
		Float4 texCoordEnd;
		TShared<TextureResource> mainTexture;
	};
}
