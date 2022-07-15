// DeferredCompact.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class DepthResolveFS : public TReflected<DepthResolveFS, IShader> {
	public:
		DepthResolveFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
		BindTexture depthTexture;
		BindBuffer uniformBuffer;
		Float4 resolveParam;

	protected:
		// inputs
		Float2 rasterCoord;

		// outputs
		Float4 outputDepth;
	};
}
