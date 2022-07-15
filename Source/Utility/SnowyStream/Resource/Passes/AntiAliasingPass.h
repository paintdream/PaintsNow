// AntiAliasingPass.h
// AntiAliasingFS Pass
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/AntiAliasingFS.h"

namespace PaintsNow {
	class AntiAliasingPass : public TReflected<AntiAliasingPass, PassBase> {
	public:
		AntiAliasingPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		AntiAliasingFS antiAliasing;
	};
}
