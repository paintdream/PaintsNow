// DeferredCompact.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class DeferredCompactEncodeFS : public TReflected<DeferredCompactEncodeFS, IShader> {
	public:
		DeferredCompactEncodeFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	protected:
		// imports
		Float4 outputColor;
		Float3 outputNormal;
		float occlusion;
		float metallic;
		float roughness;

		// outputs
		Float4 encodeBaseColorOcclusion;
		Float4 encodeNormalRoughnessMetallic;
	};

	class DeferredCompactDecodeFS : public TReflected<DeferredCompactDecodeFS, IShader> {
	public:
		DeferredCompactDecodeFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		// input
		BindTexture BaseColorOcclusionTexture;
		BindTexture NormalRoughnessMetallicTexture;
		BindTexture DepthTexture;
		BindTexture ShadowTexture;
		BindTexture ScreenTexture;
		BindTexture ReflectCoordTexture;

		BindBuffer uniformProjectionBuffer;

		MatrixFloat4x4 inverseProjectionMatrix;
		Float2 rasterCoord;

		// outputs
		Float3 screenSpaceColor;
		Float3 viewPosition;
		Float3 viewNormal;
		Float3 baseColor;
		float occlusion;
		float metallic;
		float roughness;
		float depth;
		float shadow;

		bool enableScreenSpaceColor;
	};
}
