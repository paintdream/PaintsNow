#define USE_SWIZZLE
#include "SkySingleScatteringFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkySingleScatteringFS::SkySingleScatteringFS() {}

String SkySingleScatteringFS::GetShaderText() {
	return UnifyShaderCode(
		float3 dr;
		float3 dm;
		ComputeSingleScatteringTexture( 
			atmosphere, transmittance_texture, float3(rasterCoord.x, rasterCoord.y, layer + 0.5),
			dr, dm);
		delta_rayleigh.xyz = dr;
		delta_mie.xyz = dm;
		scattering.xyz = mult_vec(luminance_from_radiance, delta_rayleigh.xyz);
		scattering.w = mult_vec(luminance_from_radiance, delta_mie.xyz).x;
		single_mie_scattering.xyz = mult_vec(luminance_from_radiance, delta_mie.xyz);
	);
}

TObject<IReflect>& SkySingleScatteringFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transmittance_texture);
		ReflectProperty(luminance_from_radiance)[paramBuffer];
		ReflectProperty(layer)[paramBuffer];

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(delta_rayleigh)[BindOutput(BindOutput::COLOR)];
		ReflectProperty(delta_mie)[BindOutput(BindOutput::COLOR)];
		ReflectProperty(scattering)[BindOutput(BindOutput::COLOR)];
		ReflectProperty(single_mie_scattering)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}