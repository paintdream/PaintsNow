#define USE_SWIZZLE
#include "SkyScatteringDensityFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyScatteringDensityFS::SkyScatteringDensityFS() {
	single_rayleigh_scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	single_mie_scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	multiple_scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
}

String SkyScatteringDensityFS::GetShaderText() {
	return UnifyShaderCode(
		float3 retValue;
		ComputeScatteringDensityTexture(retValue,
			atmosphere, transmittance_texture, single_rayleigh_scattering_texture,
			single_mie_scattering_texture, multiple_scattering_texture,
			irradiance_texture, float3(rasterCoord.x, rasterCoord.y, layer + 0.5),
			scattering_order);
		scattering_density.xyz = retValue.xyz;
	);
}

TObject<IReflect>& SkyScatteringDensityFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(scattering_order)[paramBuffer];
		ReflectProperty(layer)[paramBuffer];

		ReflectProperty(transmittance_texture);
		ReflectProperty(single_rayleigh_scattering_texture);
		ReflectProperty(single_mie_scattering_texture);
		ReflectProperty(multiple_scattering_texture);
		ReflectProperty(irradiance_texture);

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(scattering_density)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}