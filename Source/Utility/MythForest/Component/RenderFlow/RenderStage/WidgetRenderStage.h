// WidgetRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	class WidgetRenderStage : public TReflected<WidgetRenderStage, RenderStage> {
	public:
		WidgetRenderStage(const String& s);
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void Initialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;
		void OnFrameEncodeBegin(Engine& engine, RenderPortCommandQueue& commandQueue);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortCommandQueue Widgets;
		RenderPortRenderTargetLoad InputColor; // optional
		RenderPortRenderTargetStore OutputColor;
	};
}

