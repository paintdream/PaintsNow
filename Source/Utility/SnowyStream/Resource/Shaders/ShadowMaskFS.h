// ShadowMaskFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ShadowMaskFS : public TReflected<ShadowMaskFS, IShader> {
	public:
		ShadowMaskFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture shadowTexture;
		BindTexture depthTexture;
		BindBuffer uniformBuffer;

		// Uniforms
		MatrixFloat4x4 reprojectionMatrix;
		Float2 invScreenSize;
		Float2 unjitter;
		float shadowBias;

		// Inputs
		Float4 rasterCoord;

		// Outputs
		Float4 shadow;
	};
}
