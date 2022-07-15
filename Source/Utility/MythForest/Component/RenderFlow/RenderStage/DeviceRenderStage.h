// DeviceRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	class DeviceRenderStage : public TReflected<DeviceRenderStage, RenderStage> {
	public:
		DeviceRenderStage(const String& config = "1");
		void OnFrameResolutionUpdate(Engine& engine, IRender::Queue* queue, UShort2 res) override;
		void PreInitialize(Engine& engine, IRender::Queue* queue) override;
		void Uninitialize(Engine& engine, IRender::Queue* queue) override;
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& queues, IRender::Queue* instantMainQueue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortRenderTargetLoad InputColor;
		IRender::Resource::TransferDescription transferDescription;
		IRender::Resource* transferResource;
		IRender::Queue* transferQueue;
	};
}

