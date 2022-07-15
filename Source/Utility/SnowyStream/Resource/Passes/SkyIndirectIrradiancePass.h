// SkyIndirectIrradiancePass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/SkyIndirectIrradianceFS.h"

namespace PaintsNow {
	class SkyIndirectIrradiancePass : public TReflected<SkyIndirectIrradiancePass, PassBase> {
	public:
		SkyIndirectIrradiancePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		ScreenTransformVS transform;
		SkyIndirectIrradianceFS shading;
	};
}

