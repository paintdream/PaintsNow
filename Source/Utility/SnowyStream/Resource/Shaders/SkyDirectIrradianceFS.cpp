#define USE_SWIZZLE
#include "SkyDirectIrradianceFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyDirectIrradianceFS::SkyDirectIrradianceFS() {
}

String SkyDirectIrradianceFS::GetShaderText() {
	return UnifyShaderCode(
		float3 retValue;
		ComputeDirectIrradianceTexture(retValue, atmosphere, transmittance_texture, rasterCoord.xy);
		delta_irradiance.xyz = retValue;
		delta_irradiance.w = 1;
	);
}

TObject<IReflect>& SkyDirectIrradianceFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transmittance_texture);

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(delta_irradiance)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}