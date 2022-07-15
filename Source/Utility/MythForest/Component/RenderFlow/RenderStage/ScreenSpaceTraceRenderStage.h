// ScreenSpaceTraceRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/ScreenSpaceTracePass.h"

namespace PaintsNow {
	class ScreenSpaceTraceRenderStage : public TReflected<ScreenSpaceTraceRenderStage, GeneralRenderStageDraw<ScreenSpaceTracePass> > 	{
	public:
		ScreenSpaceTraceRenderStage(const String& s);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;

		RenderPortTextureInput Depth;
		RenderPortRenderTargetLoad LoadDepth;
		RenderPortTextureInput Normal;
		TRenderPortReference<RenderPortLightSource> LightSource;
		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortRenderTargetStore ScreenCoord;
	};
}

