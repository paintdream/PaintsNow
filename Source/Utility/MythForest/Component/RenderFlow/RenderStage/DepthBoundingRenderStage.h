// DepthBoundingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/DepthBoundingPass.h"

namespace PaintsNow {
	class DepthBoundingRenderStage : public TReflected<DepthBoundingRenderStage, GeneralRenderStageDraw<DepthBoundingPass> > {
	public:
		DepthBoundingRenderStage(const String& config = "1");
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputDepth;
		RenderPortRenderTargetStore OutputDepth;
	};
}

