// ScreenOutlineFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ScreenOutlineFS : public TReflected<ScreenOutlineFS, IShader> {
	public:
		ScreenOutlineFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	public:
		BindTexture inputMaskTexture;
		BindBuffer paramBuffer;

		Float3 outlineColor;
		float outlineWidth;
		Float2 invScreenSize;

	protected:
		Float2 rasterCoord;
		Float4 outputColor;
	};
}
