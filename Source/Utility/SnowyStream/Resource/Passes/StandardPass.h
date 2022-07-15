// StandardPass.h
// Standard Physically Based Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/StandardParameterFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class StandardPass : public TReflected<StandardPass, PassBase> {
	public:
		StandardPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		// Vertex shaders
		StandardTransformVS standardTransform;
		// Fragment shaders
		StandardParameterFS shaderParameter;
		DeferredCompactEncodeFS shaderCompactEncode;
	};
}
