// DeferredCompact.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class DepthMinMaxFS : public TReflected<DepthMinMaxFS, IShader> {
	public:
		DepthMinMaxFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
		BindTexture depthTexture;
		BindBuffer uniformBuffer;
		Float2 invScreenSize;

	protected:
		// inputs
		Float2 rasterCoord;

		// outputs
		Float4 outputDepth;
	};
}
