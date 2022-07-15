// FrameBarrierRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	class FrameBarrierRenderStage : public TReflected<FrameBarrierRenderStage, RenderStage> {
	public:
		FrameBarrierRenderStage(const String& s);

		void OnFrameResolutionUpdate(Engine& engine, IRender::Queue* queue, UShort2 res) override;
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;
		void OnFrameTick(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& queues, IRender::Queue* instantMainQueue) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput Front;
		RenderPortRenderTargetStore Next;
	};
}

