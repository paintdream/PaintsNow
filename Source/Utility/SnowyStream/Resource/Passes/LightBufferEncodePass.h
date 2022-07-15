// LightBufferEncodePass.h
// LightBufferFS Pass
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/LightEncoderCS.h"

namespace PaintsNow {
	class LightBufferEncodePass : public TReflected<LightBufferEncodePass, PassBase> {
	public:
		LightBufferEncodePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Compute shader
		LightEncoderCS encoder;
	};
}
