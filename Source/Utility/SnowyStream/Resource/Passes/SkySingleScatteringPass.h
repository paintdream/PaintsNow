// SkySingleScatteringPass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/SkySingleScatteringFS.h"

namespace PaintsNow {
	class SkySingleScatteringPass : public TReflected<SkySingleScatteringPass, PassBase> {
	public:
		SkySingleScatteringPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		ScreenTransformVS transform;
		SkySingleScatteringFS shading;
	};
}

