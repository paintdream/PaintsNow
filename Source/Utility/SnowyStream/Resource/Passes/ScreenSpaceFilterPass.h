// ScreenSpaceFilterPass.h
// ScreenFS Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/ScreenSpaceFilterFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class ScreenSpaceFilterPass : public TReflected<ScreenSpaceFilterPass, PassBase> {
	public:
		ScreenSpaceFilterPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		ScreenSpaceFilterFS shaderScreen;
	};
}
