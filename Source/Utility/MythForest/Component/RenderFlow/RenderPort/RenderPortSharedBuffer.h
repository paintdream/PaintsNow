// RenderPortSharedBuffer.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "RenderPortRenderTarget.h"

namespace PaintsNow {
	class RenderPortSharedBufferStore;
	class RenderPortSharedBufferLoad : public TReflected<RenderPortSharedBufferLoad, RenderPort> {
	public:
		RenderPortSharedBufferLoad(bool save = false);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		void OnFramePreTick(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;

		IRender::Resource* sharedBufferResource;
		UShort2 bufferSize;
		UShort2 depthSize;
	};

	class RenderPortSharedBufferStore : public TReflected<RenderPortSharedBufferStore, RenderPort> {
	public:
		RenderPortSharedBufferStore();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		void OnFramePreTick(Engine& engine, IRender::Queue* queue) override;

		IRender::Resource* sharedBufferResource;
		UShort2 bufferSize;
		UShort2 depthSize;
	};
}

