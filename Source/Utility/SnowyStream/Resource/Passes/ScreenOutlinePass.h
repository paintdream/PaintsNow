// ScreenOutlinePass.h
// ScreenFS Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/ScreenOutlineFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class ScreenOutlinePass : public TReflected<ScreenOutlinePass, PassBase> {
	public:
		ScreenOutlinePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		ScreenOutlineFS shaderScreen;
	};
}
