// ScreenSpaceTracePass.h
// ScreenFS Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/ScreenSpaceTraceFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class ScreenSpaceTracePass : public TReflected<ScreenSpaceTracePass, PassBase> {
	public:
		ScreenSpaceTracePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		ScreenSpaceTraceFS shaderScreen;
	};
}
