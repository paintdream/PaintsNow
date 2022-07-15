// WidgetShadingFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class WidgetShadingFS : public TReflected<WidgetShadingFS, IShader> {
	public:
		WidgetShadingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	public:
		IShader::BindTexture mainTexture;

	protected:
		// varyings
		Float4 texCoord;

		// targets
		Float4 target;
	};
}

