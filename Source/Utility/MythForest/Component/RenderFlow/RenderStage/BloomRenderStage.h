// BloomRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/BloomPass.h"

namespace PaintsNow {
	class BloomRenderStage : public TReflected<BloomRenderStage, GeneralRenderStageDraw<BloomPass> > {
	public:
		BloomRenderStage(const String& config);
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputColor;
		RenderPortRenderTargetStore OutputColor;
	};
}

