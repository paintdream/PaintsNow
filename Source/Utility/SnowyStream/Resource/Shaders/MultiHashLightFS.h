// MultiHashLightFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class MultiHashLightFS : public TReflected<MultiHashLightFS, IShader> {
	public:
		MultiHashLightFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture depthTexture;
		BindTexture lightDepthTexture;
		BindBuffer lightParamBuffer;

		Float2 rasterCoord;
		MatrixFloat4x4 invProjectionMatrix;
		MatrixFloat4x4 lightProjectionMatrix;
		Float3 lightColor;
		float lightAttenuation;

		// output
		Float4 blendColor;
	};
}
