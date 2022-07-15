// LightTextureEncodePass.h
// LightBufferFS Pass
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/LightEncoderFS.h"

namespace PaintsNow {
	class LightTextureEncodePass : public TReflected<LightTextureEncodePass, PassBase> {
	public:
		LightTextureEncodePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		LightEncoderFS encoder;
	};
}
