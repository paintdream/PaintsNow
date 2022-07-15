// SkyMultipleScatteringPass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/SkyMultipleScatteringFS.h"

namespace PaintsNow {
	class SkyMultipleScatteringPass : public TReflected<SkyMultipleScatteringPass, PassBase> {
	public:
		SkyMultipleScatteringPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		ScreenTransformVS transform;
		SkyMultipleScatteringFS shading;
	};
}

