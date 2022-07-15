// TextShadingFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class TextShadingFS : public TReflected<TextShadingFS, IShader> {
	public:
		TextShadingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	public:
		IShader::BindTexture mainTexture;
		Float4 color;
		Float2 texCoord;

		// targets
		Float4 target;
	};
}

