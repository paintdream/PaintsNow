// RenderPort.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-24
//

#pragma once
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../../Core/System/Graph.h"

namespace PaintsNow {
	class Engine;
	class RenderPort : public TReflected<RenderPort, GraphPort<SharedTiny> > {
	public:
		enum { RENDERPORT_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN };

		TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual void Initialize(Engine& engine, IRender::Queue* queue);
		virtual void Uninitialize(Engine& engine, IRender::Queue* queue);
		virtual bool OnFrameEncodeBegin(Engine& engine);
		virtual void OnFrameEncodeEnd(Engine& engine);
		virtual void OnFramePreTick(Engine& engine, IRender::Queue* queue);
		virtual void OnFrameUpdate(Engine& engine, IRender::Queue* queue);
		virtual void OnFramePostTick(Engine& engine, IRender::Queue* queue);
		virtual uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue);
		virtual uint32_t OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& repeatQueues, IRender::Queue* instantMainQueue);
		void UpdateRenderStage();

		std::vector<String> publicSymbols;
		TEvent<Engine&, RenderPort&, IRender::Queue*> eventTickHooks;
	};

	template <class T>
	class TRenderPortReference : public TReflected<TRenderPortReference<T>, RenderPort> {
	public:
		TRenderPortReference() : targetRenderPort(nullptr) {}
		void Initialize(Engine& engine, IRender::Queue* mainQueue) override {}
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override {}

		void OnFramePreTick(Engine& engine, IRender::Queue* queue) override {
			if (!RenderPort::GetLinks().empty()) {
				T* port = RenderPort::GetLinks().back().port->QueryInterface(UniqueType<T>());
				assert(port != nullptr);
				if (port != nullptr) {
					targetRenderPort = port;
				}
			}

			RenderPort::Flag().store(targetRenderPort->Flag().load(std::memory_order_relaxed), std::memory_order_relaxed);
		}

		T* operator -> () {
			return targetRenderPort;
		}

		const T* operator -> () const {
			return targetRenderPort;
		}

		T* targetRenderPort;
	};

	class RenderPortParameterAdapter : public TReflected<RenderPortParameterAdapter, RenderPort> {
	public:
		virtual PassBase::Updater& GetUpdater() = 0;
	};

	template <class T>
	class RenderPortShaderPass : public TReflected<RenderPortShaderPass<T>, RenderPortParameterAdapter> {
	public:
		RenderPortShaderPass(const TShared<ShaderResourceImpl<T> >& s) : shaderResource(s) {}
		void Initialize(Engine& engine, IRender::Queue* mainQueue) override {}
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override {}
		PassBase::Updater& GetUpdater() override { return shaderResource->GetPassUpdater(); }

		inline T& GetPass() {
			return static_cast<T&>(shaderResource->GetPass());
		}

	private:
		TShared<ShaderResourceImpl<T> > shaderResource;
	};
}
