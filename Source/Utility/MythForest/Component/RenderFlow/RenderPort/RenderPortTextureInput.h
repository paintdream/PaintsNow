// RenderPortTextureInput.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"

namespace PaintsNow {
	class RenderStage;
	class RenderPortTextureInput : public TReflected<RenderPortTextureInput, RenderPort> {
	public:
		RenderPortTextureInput();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		TShared<TextureResource> textureResource;

		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		void OnFramePreTick(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;
	};
}

