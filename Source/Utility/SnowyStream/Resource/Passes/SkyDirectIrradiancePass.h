// SkyDirectIrradiancePass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/SkyDirectIrradianceFS.h"

namespace PaintsNow {
	class SkyDirectIrradiancePass : public TReflected<SkyDirectIrradiancePass, PassBase> {
	public:
		SkyDirectIrradiancePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		ScreenTransformVS transform;
		SkyDirectIrradianceFS shading;
	};
}

