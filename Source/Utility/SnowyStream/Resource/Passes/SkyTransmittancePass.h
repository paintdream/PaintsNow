// SkyTransmittancePass.h
// Standard Sky Shader
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/SkyTransmittanceFS.h"

namespace PaintsNow {
	class SkyTransmittancePass : public TReflected<SkyTransmittancePass, PassBase> {
	public:
		SkyTransmittancePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		ScreenTransformVS transform;
		SkyTransmittanceFS shading;
	};
}

