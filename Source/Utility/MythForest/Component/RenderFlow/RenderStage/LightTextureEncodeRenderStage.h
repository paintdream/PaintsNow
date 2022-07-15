// LightTextureEncodeRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/LightTextureEncodePass.h"

namespace PaintsNow {
	class LightTextureEncodeRenderStage : public TReflected<LightTextureEncodeRenderStage, GeneralRenderStageDraw<LightTextureEncodePass> > {
	public:
		LightTextureEncodeRenderStage(const String& config = "1");
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;
		void Uninitialize(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortLightSource LightSource;
		RenderPortTextureInput InputDepth;
		RenderPortRenderTargetStore LightTexture;
	};
}

