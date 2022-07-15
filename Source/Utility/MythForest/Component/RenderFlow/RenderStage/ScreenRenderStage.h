// ScreenRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/ScreenPass.h"

namespace PaintsNow {
	class ScreenRenderStage : public TReflected<ScreenRenderStage, GeneralRenderStageDraw<ScreenPass> > {
	public:
		ScreenRenderStage(const String& config = "1");
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputColor;
		RenderPortRenderTargetStore OutputColor;

		std::vector<TShared<RenderPortTextureInput> > BloomLayers;
	};
}

