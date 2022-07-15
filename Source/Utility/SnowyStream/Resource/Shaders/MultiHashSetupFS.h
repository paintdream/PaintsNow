// MultiHashSetupFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class MultiHashSetupFS : public TReflected<MultiHashSetupFS, IShader> {
	public:
		MultiHashSetupFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindBuffer setupParamBuffer;
		BindTexture noiseTexture;

		BindTexture lightDepthTexture;
		MatrixFloat4x4 lightReprojectionMatrix;
		Float4 lightColor;
		Float4 lightPosition;
		Float2 invScreenSize;

		Float4 rasterCoord;
		Float4 tintColor;

		// output
		Float4 outputIrradiance;
	};
}
