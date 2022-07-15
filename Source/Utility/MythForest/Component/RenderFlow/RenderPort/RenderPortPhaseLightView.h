// RenderPortPhaseLightView.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Light/LightComponent.h"
#include <queue>

namespace PaintsNow {
	class RenderPortPhaseLightView : public TReflected<RenderPortPhaseLightView, RenderPort> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortPhaseLightView();

		class PhaseInfo {
		public:
			MatrixFloat4x4 viewMatrix;
			MatrixFloat4x4 projectionMatrix;
			TShared<TextureResource> irradiance;
			TShared<TextureResource> depth;
		};

		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		bool OnFrameEncodeBegin(Engine& engine) override;
		void OnFrameEncodeEnd(Engine& engine) override;

		std::vector<PhaseInfo> phases;
	};
}

