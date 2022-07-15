// StandardLightingBufferEncodedFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class StandardLightingBufferEncodedFS : public TReflected<StandardLightingBufferEncodedFS, IShader> {
	public:
		StandardLightingBufferEncodedFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
		String GetEnvironmentLighting(float& retValue, float intensity);

		BindTexture specTexture;
		BindBuffer lightInfoBuffer;
		BindBuffer lightIndexBuffer;
		BindBuffer paramBuffer;

		Float3 screenSpaceColor;
		Float3 viewPosition;
		Float3 viewNormal;
		Float3 baseColor;
		float metallic;
		float roughness;
		float occlusion;
		float shadow;

		enum { MAX_LIGHT_COUNT = 64 };
		std::vector<Float4> lightInfos; // position & color
		std::vector<uint32_t> lightIndices;
		uint32_t lightCount;
		MatrixFloat4x4 invWorldNormalMatrix;
		float cubeLevelInv;
		float cubeStrength;
		Float2 depthTextureSize;
		Float2 lightBufferSize;
		Float2 reserved;
		
		Float2 rasterCoord; // imported
		Float4 mainColor; // target
	};
}
