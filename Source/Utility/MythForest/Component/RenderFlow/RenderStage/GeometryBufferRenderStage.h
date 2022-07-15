// GeometryBufferRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortCameraView.h"

namespace PaintsNow {
	class GeometryBufferRenderStage : public TReflected<GeometryBufferRenderStage, RenderStage> {
	public:
		GeometryBufferRenderStage(const String& s);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;
		void OnFrameEncodeBegin(Engine& engine, RenderPortCommandQueue& commandQueue);

		RenderPortCameraView CameraView;
		RenderPortCommandQueue Primitives;

		RenderPortRenderTargetStore BaseColorOcclusion;
		RenderPortRenderTargetStore NormalRoughnessMetallic;
		RenderPortRenderTargetStore Depth;
	};
}

