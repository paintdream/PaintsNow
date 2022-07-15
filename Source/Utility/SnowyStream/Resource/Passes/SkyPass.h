// SkyPass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/SkyTransformVS.h"
#include "../Shaders/SkyShadingFS.h"

namespace PaintsNow {
	class SkyPass : public TReflected<SkyPass, PassBase> {
	public:
		SkyPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		SkyTransformVS transform;
		SkyShadingFS shading;
	};
}

