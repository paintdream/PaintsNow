// ScreenOutlineRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	class ScreenOutlineRenderStage : public TReflected<ScreenOutlineRenderStage, RenderStage> {
	public:
		ScreenOutlineRenderStage(const String& s);
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortRenderTargetLoad InputColor; // optional
		RenderPortRenderTargetStore OutputColor;
	};
}

