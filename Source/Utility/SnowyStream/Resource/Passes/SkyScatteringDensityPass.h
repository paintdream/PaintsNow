// SkyScatteringDensityPass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/SkyScatteringDensityFS.h"

namespace PaintsNow {
	class SkyScatteringDensityPass : public TReflected<SkyScatteringDensityPass, PassBase> {
	public:
		SkyScatteringDensityPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		ScreenTransformVS transform;
		SkyScatteringDensityFS shading;
	};
}

