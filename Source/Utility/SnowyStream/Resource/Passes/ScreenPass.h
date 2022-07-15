// ScreenPass.h
// ScreenFS Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/ScreenFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class ScreenPass : public TReflected<ScreenPass, PassBase> {
	public:
		ScreenPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		ScreenFS shaderScreen;
	};
}
