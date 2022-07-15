// CustomMaterialPass.h
// Standard Physical Based Shader
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/CustomMaterialTransformVS.h"
#include "../Shaders/CustomMaterialParameterFS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "../TextureResource.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx brdf
	class CustomMaterialPass : public TReflected<CustomMaterialPass, PassBase> {
	public:
		CustomMaterialPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void SetInput(const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) override;
		void SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) override;
		void SetComplete() override;
		bool FlushOptions() override;
		void DetachDescription();

		// Vertex shaders
		CustomMaterialTransformVS shaderTransform;
		// Fragment shaders
		CustomMaterialParameterFS shaderParameter;
		DeferredCompactEncodeFS shaderCompactEncode;
	};
}
