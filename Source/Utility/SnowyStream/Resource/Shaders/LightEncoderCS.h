// DeferredCompact.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class LightEncoderCS : public TReflected<LightEncoderCS, IShader> {
	public:
		LightEncoderCS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		UInt3 computeGroup;
		BindTexture depthTexture;
		BindBuffer lightInfoBuffer;
		enum { MAX_LIGHT_COUNT = 120 };

		// uniforms
		Float4 inverseProjectionParams;
		Float2 invScreenSize;
		float lightCount;
		float reserved;
		std::vector<Float4> lightInfos;

		// outputs
		IShader::BindBuffer lightBuffer;
		std::vector<uint32_t> lightBufferData;
	};
}
