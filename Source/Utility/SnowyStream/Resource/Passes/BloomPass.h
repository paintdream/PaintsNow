// BloomPass.h
// BloomFS Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/BloomFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class BloomPass : public TReflected<BloomPass, PassBase> {
	public:
		BloomPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		BloomFS screenBloom;
	};
}
