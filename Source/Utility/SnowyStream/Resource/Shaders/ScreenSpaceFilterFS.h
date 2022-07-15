// ScreenSpaceFilterFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ScreenSpaceFilterFS : public TReflected<ScreenSpaceFilterFS, IShader> {
	public:
		ScreenSpaceFilterFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture inputTexture;
		BindBuffer filterBuffer;
		Float2 invScreenSize;

		// varying
		Float4 rasterCoord;

		// output
		Float4 filteredValue;
	};
}
