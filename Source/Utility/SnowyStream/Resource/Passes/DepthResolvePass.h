// DepthResolvePass.h
// DepthResolveFS Pass
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DepthResolveFS.h"

namespace PaintsNow {
	class DepthResolvePass : public TReflected<DepthResolvePass, PassBase> {
	public:
		DepthResolvePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		DepthResolveFS resolve;
	};
}
