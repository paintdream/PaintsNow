// ForwardLightingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../../../../SnowyStream/Resource/Passes/ForwardLightingPass.h"

namespace PaintsNow {
	class ForwardLightingRenderStage : public TReflected<ForwardLightingRenderStage, RenderStage> {
	public:
		ForwardLightingRenderStage(const String& s);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void Initialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;
		void OnFrameEncodeBegin(Engine& engine, RenderPortCommandQueue& commandQueue);

		RenderPortLightSource LightSource;
		RenderPortCommandQueue Primitives; // input primitives

		RenderPortRenderTargetLoad InputColor;
		RenderPortRenderTargetLoad InputDepth;
		RenderPortRenderTargetStore OutputColor;
		RenderPortRenderTargetStore OutputDepth;
	};
}

