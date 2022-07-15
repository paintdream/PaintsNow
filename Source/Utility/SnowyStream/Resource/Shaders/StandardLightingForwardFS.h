// StandardLightingForwardFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class StandardLightingForwardFS : public TReflected<StandardLightingForwardFS, IShader> {
	public:
		StandardLightingForwardFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture specTexture;
		BindBuffer lightInfoBuffer;
		BindBuffer paramBuffer;

		Float3 viewPosition;
		Float3 outputNormal;
		Float3 outputColor;
		float metallic;
		float roughness;
		float occlusion;
		float alpha;
		float shadow;

		enum { MAX_LIGHT_COUNT = 4 };
		std::vector<Float4> lightInfos; // position & color
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
