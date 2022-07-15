// DepthBoundingSetupPass.h
// DepthBoundingSetupFS Pass
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DepthMinMaxSetupFS.h"

namespace PaintsNow {
	class DepthBoundingSetupPass : public TReflected<DepthBoundingSetupPass, PassBase> {
	public:
		DepthBoundingSetupPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		DepthMinMaxSetupFS minmax;
	};
}
