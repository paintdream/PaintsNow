// SurfaceComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"

namespace PaintsNow {
	class SurfaceComponent : public TAllocatedTiny<SurfaceComponent, RenderableComponent> {
	public:
		SurfaceComponent();
		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;
	};
}
