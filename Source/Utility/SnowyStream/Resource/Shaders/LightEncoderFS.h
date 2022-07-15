// LightEncoder.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class LightEncoderFS : public TReflected<LightEncoderFS, IShader> {
	public:
		LightEncoderFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
		BindTexture depthTexture;
		BindBuffer lightInfoBuffer;
		enum { MAX_LIGHT_COUNT = 120 };

		// uniforms
		Float4 inverseProjectionParams;
		Float3 reserved;
		float lightCount;
		std::vector<Float4> lightInfos;

	protected:
		// inputs
		Float2 rasterCoord;

		// outputs
		Float4 outputIndex; // RGBA16UInt
	};
}
