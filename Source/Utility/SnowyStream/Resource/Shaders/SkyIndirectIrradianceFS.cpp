#define USE_SWIZZLE
#include "SkyIndirectIrradianceFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyIndirectIrradianceFS::SkyIndirectIrradianceFS() {
	single_rayleigh_scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	single_mie_scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	multiple_scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
}

String SkyIndirectIrradianceFS::GetShaderText() {
	return UnifyShaderCode(
		float3 retValue;
		ComputeIndirectIrradianceTexture(retValue, 
			atmosphere, single_rayleigh_scattering_texture,
			single_mie_scattering_texture, multiple_scattering_texture,
			rasterCoord.xy, scattering_order);
		delta_irradiance.xyz = retValue;
		irradiance.xyz = mult_vec(luminance_from_radiance, delta_irradiance.xyz);
	);
}

TObject<IReflect>& SkyIndirectIrradianceFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(single_rayleigh_scattering_texture);
		ReflectProperty(single_mie_scattering_texture);
		ReflectProperty(multiple_scattering_texture);
		ReflectProperty(luminance_from_radiance)[paramBuffer];
		ReflectProperty(scattering_order)[paramBuffer];

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(delta_irradiance)[BindOutput(BindOutput::COLOR)];
		ReflectProperty(irradiance)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}