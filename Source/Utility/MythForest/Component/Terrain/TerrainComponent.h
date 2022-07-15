// TerrainComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../SnowyStream/Resource/TerrainResource.h"

namespace PaintsNow {
	class TerrainComponent : public TAllocatedTiny<TerrainComponent, RenderableComponent> {
	public:
		TerrainComponent();
		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;

	protected:
		TShared<TerrainResource> terrainResource;
	};
}
