// DeferredLightingTextureEncodedRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/DeferredLightingTextureEncodedPass.h"

namespace PaintsNow {
	class DeferredLightingTextureEncodedRenderStage : public TReflected<DeferredLightingTextureEncodedRenderStage, GeneralRenderStageDraw<DeferredLightingTextureEncodedPass> > {
	public:
		DeferredLightingTextureEncodedRenderStage(const String& s);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;

		TRenderPortReference<RenderPortCameraView> CameraView;
		TRenderPortReference<RenderPortLightSource> LightSource;

		RenderPortTextureInput BaseColorOcclusion;			// Base.x, Base.y, Base.z, Metallic
		RenderPortTextureInput NormalRoughnessMetallic;		// N.x, N.y, Roughness, Occlusion
		RenderPortTextureInput Depth;
		RenderPortTextureInput LightTexture;
		RenderPortTextureInput ShadowTexture;

		RenderPortTextureInput LastInputColor;
		RenderPortTextureInput ReflectCoord;
		RenderPortRenderTargetLoad LoadDepth;
		RenderPortRenderTargetStore OutputColor;
	};
}

