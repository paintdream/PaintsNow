// SurfaceVS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class SurfaceVS : public TReflected<SurfaceVS, IShader> {
	public:
		SurfaceVS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
	};
}
