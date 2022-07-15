// SkyMapPass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/SkyTransformVS.h"
#include "../Shaders/SkyMapShadingFS.h"

namespace PaintsNow {
	class SkyMapPass : public TReflected<SkyMapPass, PassBase> {
	public:
		SkyMapPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		SkyTransformVS transform;
		SkyMapShadingFS shading;
	};
}

