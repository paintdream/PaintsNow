#define USE_SWIZZLE
#include "SkyMultipleScatteringFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyMultipleScatteringFS::SkyMultipleScatteringFS() {
	scattering_density_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
}

String SkyMultipleScatteringFS::GetShaderText() {
	return UnifyShaderCode(
		float nu;
	float3 retValue;
	ComputeMultipleScatteringTexture(retValue,
		atmosphere, transmittance_texture, scattering_density_texture,
		float3(rasterCoord.x, rasterCoord.y, layer + 0.5), nu);
	delta_multiple_scattering.xyz = retValue;
	float r;
	RayleighPhaseFunction(r, nu);
	scattering.xyz = mult_vec(luminance_from_radiance,
		delta_multiple_scattering.xyz) / r;
	scattering.w = 0;
	);
}

TObject<IReflect>& SkyMultipleScatteringFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(luminance_from_radiance)[paramBuffer];
		ReflectProperty(layer)[paramBuffer];
		ReflectProperty(transmittance_texture);
		ReflectProperty(scattering_density_texture);

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(delta_multiple_scattering)[BindOutput(BindOutput::COLOR)];
		ReflectProperty(scattering)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}