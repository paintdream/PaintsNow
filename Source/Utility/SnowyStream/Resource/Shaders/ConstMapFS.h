// ConstMapFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ConstMapFS : public TReflected<ConstMapFS, IShader> {
	public:
		ConstMapFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	protected:
		BindTexture baseColorTexture;
		// varyings
		Float4 tintColor;
		Float4 texCoord;

		// targets
		Float4 target;

		bool enableBaseColorTint;
		bool enableBaseColorTexture;
		bool enableAlphaTest;
	};
}

