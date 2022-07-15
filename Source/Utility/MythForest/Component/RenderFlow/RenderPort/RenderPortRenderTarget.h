// RenderPortRenderTarget.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"

namespace PaintsNow {
	class MetaAdaptMainResolution : public MetaNodeBase {
	public:
		MetaAdaptMainResolution(int8_t bitwiseShift = 0);
		MetaAdaptMainResolution operator = (int8_t bitwiseShift);

		template <class T, class D>
		inline const MetaAdaptMainResolution& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaAdaptMainResolution Type;
		};

		typedef MetaAdaptMainResolution Type;

		uint8_t bitwiseShift;
	};

	extern MetaAdaptMainResolution AdaptMainResolution;

	class RenderPortRenderTargetStore;
	class RenderPortRenderTargetLoad : public TReflected<RenderPortRenderTargetLoad, RenderPort> {
	public:
		RenderPortRenderTargetLoad(IRender::Resource::RenderTargetDescription::Storage& storage, bool save = false);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		void OnFramePreTick(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;

		RenderPortRenderTargetStore* QueryStore() const;
		IRender::Resource::RenderTargetDescription::Storage& bindingStorage;
	};

	class RenderPortRenderTargetStore : public TReflected<RenderPortRenderTargetStore, RenderPort> {
	public:
		RenderPortRenderTargetStore(IRender::Resource::RenderTargetDescription::Storage& storage);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		void OnFramePreTick(Engine& engine, IRender::Queue* queue) override;
		uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) override;

		RenderPortRenderTargetLoad* QueryLoad() const;

		IRender::Resource::RenderTargetDescription::Storage& bindingStorage;
		TShared<TextureResource> attachedTexture;
		IRender::Resource::TextureDescription renderTargetDescription;
	};
}

