// DeferredLightingBufferEncoded.h
// Standard Physical Based Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "../Shaders/StandardLightingBufferEncodedFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx brdf
	class DeferredLightingBufferEncodedPass : public TReflected<DeferredLightingBufferEncodedPass, PassBase> {
	public:
		DeferredLightingBufferEncodedPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		DeferredCompactDecodeFS deferredCompactDecode;
		StandardLightingBufferEncodedFS standardLighting;
	};
}
