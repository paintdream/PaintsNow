// PhaseLightRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortPhaseLightView.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/MultiHashTracePass.h"

namespace PaintsNow {
	class PhaseLightRenderStage : public TReflected<PhaseLightRenderStage, GeneralRenderStageDraw<MultiHashTracePass> > {
	public:
		PhaseLightRenderStage(const String& s);
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;
		void OnFrameTick(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortTextureInput InputColor;
		RenderPortTextureInput Depth;
		RenderPortTextureInput BaseColorOcclusion;
		RenderPortTextureInput NormalRoughnessMetallic;
		RenderPortPhaseLightView PhaseLightView;
		RenderPortRenderTargetStore OutputColor;
	};
}

