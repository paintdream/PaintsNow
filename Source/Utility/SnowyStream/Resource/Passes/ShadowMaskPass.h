// ShadowMaskPass.h
// ShadowMaskFS Pass
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/ShadowMaskFS.h"

namespace PaintsNow {
	class ShadowMaskPass : public TReflected<ShadowMaskPass, PassBase> {
	public:
		ShadowMaskPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		ShadowMaskFS mask;
	};
}
