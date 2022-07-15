// DeferredLightingTextureEncoded.h
// Standard Physical Based Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "../Shaders/StandardLightingTextureEncodedFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx brdf
	class DeferredLightingTextureEncodedPass : public TReflected<DeferredLightingTextureEncodedPass, PassBase> {
	public:
		DeferredLightingTextureEncodedPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		DeferredCompactDecodeFS deferredCompactDecode;
		StandardLightingTextureEncodedFS standardLighting;
	};
}
