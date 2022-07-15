// DepthBoundingPass.h
// DepthBoundingFS Pass
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DepthMinMaxFS.h"

namespace PaintsNow {
	class DepthBoundingPass : public TReflected<DepthBoundingPass, PassBase> {
	public:
		DepthBoundingPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		DepthMinMaxFS minmax;
	};
}
