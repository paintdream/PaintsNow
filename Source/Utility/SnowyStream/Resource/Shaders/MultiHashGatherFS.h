// MultiHashGatherFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class MultiHashGatherFS : public TReflected<MultiHashGatherFS, IShader> {
	public:
		MultiHashGatherFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture depthTexture;

		// ref data
		BindTexture refDepthTexture;
		BindTexture refBaseColorOcclusionTexture;
		BindTexture refNormalRoughnessMetallicTexture;

		BindBuffer gatherParamBuffer;

		Float2 rasterCoord;

		// acc lit
		Float4 blendColor;
	};
}
