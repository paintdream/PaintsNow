// RenderPortLightSource.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Light/LightComponent.h"

namespace PaintsNow {
	class RenderPortLightSource : public TReflected<RenderPortLightSource, RenderPort> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;

		struct LightElement {
			Float4 position;
			Float4 colorAttenuation;

			class Shadow {
			public:
				MatrixFloat4x4 shadowMatrix;
				TShared<TextureResource> shadowTexture;
			};

			std::vector<Shadow> shadows;
		};

		class EnvCubeElement {
		public:
			TShared<TextureResource> cubeMapTexture;
			TShared<TextureResource> skyMapTexture;
			Float3 position;
			float cubeStrength;
		};

		RenderPortLightSource();

		void Initialize(Engine& engine, IRender::Queue* mainQueue) override;
		void Uninitialize(Engine& engine, IRender::Queue* mainQueue) override;
		bool OnFrameEncodeBegin(Engine& engine) override;
		void OnFrameEncodeEnd(Engine& engine) override;

		std::vector<LightElement> lightElements;
		TShared<TextureResource> cubeMapTexture;
		TShared<TextureResource> skyMapTexture;
		float cubeStrength;
		uint8_t stencilMask;
		uint8_t stencilShadow;
		uint8_t reserved[2];
	};
}

