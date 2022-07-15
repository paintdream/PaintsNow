#define USE_SWIZZLE
#include "CustomMaterialParameterFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

CustomMaterialParameterFS::CustomMaterialParameterFS() {
	description = TShared<CustomMaterialDescription>::From(new CustomMaterialDescription());
}

String CustomMaterialParameterFS::GetShaderText() {
	if (description->code.empty()) {
		return UnifyShaderCode(
			outputColor = float3(1.0, 1.0, 1.0);
			alpha = 1.0;
			outputNormal = float3(0.0, 0.0, 1.0);
			metallic = 0.0;
			roughness = 1.0;
			occlusion = 1.0;
		);
	} else {
		return description->code;
	}
}

TObject<IReflect>& CustomMaterialParameterFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		description->ReflectExternal(reflect, instanceData);

		ReflectProperty(outputColor)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(outputNormal)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(alpha)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(metallic)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(roughness)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(occlusion)[BindOutput(BindOutput::LOCAL)];
	}

	return *this;
}
