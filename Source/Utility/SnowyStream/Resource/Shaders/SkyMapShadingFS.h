// SkyMapShadingFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "SkyCommonFS.h"

namespace PaintsNow {
	class SkyMapShadingFS : public TReflected<SkyMapShadingFS, IShader> {
	public:
		SkyMapShadingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		// Uniform
		BindTexture skyMapTexture;
		
		// Input
		Float3 worldPosition;

		// Output
		Float4 outputColor;
	};
}

