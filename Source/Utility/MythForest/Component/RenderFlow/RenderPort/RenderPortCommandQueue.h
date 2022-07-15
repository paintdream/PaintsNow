// RenderPortCommandQueue.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Renderable/RenderableComponent.h"

namespace PaintsNow {
	class BatchComponent;
	class RenderPortCommandQueue : public TReflected<RenderPortCommandQueue, RenderPort> {
	public:
		RenderPortCommandQueue();
		~RenderPortCommandQueue() override;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		uint32_t OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& repeatQueues, IRender::Queue* instantMainQueue) override;
		void OnFramePostTick(Engine& engine, IRender::Queue* mainQueue) override;

		bool OnFrameEncodeBegin(Engine& engine) override;
		void OnFrameEncodeEnd(Engine& engine) override;
		void MergeQueue(IRender& render, IRender::Queue* instanceQueue, bool instant);
		void DrawElement(IRender& render, IRender::Resource* drawCallResource);
		void CheckinState(IRender& render, IRender::Resource* stateResource);
		IRender::Queue* GetRepeatQueue() const;

		TWrapper<void, Engine&, RenderPortCommandQueue&> CallbackFrameBegin;
		TWrapper<void, Engine&, RenderPortCommandQueue&> CallbackFrameEnd;

	protected:
		IRender::Queue* repeatQueue;
		uint32_t encodeCount;
		std::vector<IRender::Queue*> instantQueues;
	};
}

