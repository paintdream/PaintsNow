// DeferredCompact.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class AntiAliasingFS : public TReflected<AntiAliasingFS, IShader> {
	public:
		AntiAliasingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
		BindTexture inputTexture;
		BindTexture lastInputTexture;
		BindTexture depthTexture;
		BindBuffer uniformBuffer;

		MatrixFloat4x4 reprojectionMatrix;
		Float2 invScreenSize;
		Float2 unjitter;
		float lastRatio;

	protected:
		// inputs
		Float2 rasterCoord;

		// outputs
		Float4 outputColor;
	};
}
