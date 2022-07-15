// LightBufferEncodeRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortSharedBuffer.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/LightBufferEncodePass.h"

namespace PaintsNow {
	class LightBufferEncodeRenderStage : public TReflected<LightBufferEncodeRenderStage, GeneralRenderStageDraw<LightBufferEncodePass> > {
	public:
		LightBufferEncodeRenderStage(const String& config = "1");
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;
		void Uninitialize(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortLightSource LightSource;
		RenderPortTextureInput InputDepth;
		RenderPortSharedBufferStore LightBuffer;

		uint32_t lastBufferLength;
	};
}

