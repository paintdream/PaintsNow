#define USE_SWIZZLE
#include "SkyShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyShadingFS::SkyShadingFS() {
	transmittance_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	single_mie_scattering_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	irradiance_texture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String SkyShadingFS::GetSolarRadiance(RadianceSpectrum& retValue, const AtmosphereParameters& atmosphere) {
	return UnifyShaderCode(
		atmosphere.solar_irradiance /
		(PI * atmosphere.sun_angular_radius * atmosphere.sun_angular_radius);
	);
}

String SkyShadingFS::GetSkyRadianceShading(RadianceSpectrum& retValue, const AtmosphereParameters& atmosphere,
	Position camera, Direction view_ray, Length shadow_length,
	Direction sun_direction, DimensionlessSpectrum& transmittance) {
	return UnifyShaderCode(
		GetSkyRadiance(retValue, atmosphere, transmittance_texture,
			scattering_texture, single_mie_scattering_texture,
			camera, view_ray, shadow_length, sun_direction, transmittance);
	);
}

String SkyShadingFS::GetSkyRadianceToPointShading(RadianceSpectrum& retValue, const AtmosphereParameters& atmosphere,
	Position camera, Position point, Length shadow_length,
	Direction sun_direction, DimensionlessSpectrum& transmittance) {
	return UnifyShaderCode(
		GetSkyRadianceToPoint(retValue, atmosphere, transmittance_texture,
			scattering_texture, single_mie_scattering_texture,
			camera, point, shadow_length, sun_direction, transmittance);
	);
}

String SkyShadingFS::GetSunAndSkyIrradianceShading(IrradianceSpectrum& retValue, const AtmosphereParameters& atmosphere,
	Position p, Direction normal, Direction sun_direction,
	IrradianceSpectrum& sky_irradiance) {
	return UnifyShaderCode(
		GetSunAndSkyIrradiance(retValue, atmosphere, transmittance_texture,
			irradiance_texture, p, normal, sun_direction, sky_irradiance);
	);
}

String SkyShadingFS::GetSolarLuminance(Luminance3& retValue, const AtmosphereParameters& atmosphere) {
	return UnifyShaderCode(
		retValue = atmosphere.solar_irradiance /
		(PI * atmosphere.sun_angular_radius * atmosphere.sun_angular_radius) *
		SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
	);
}
String SkyShadingFS::GetSkyLuminanceShading(Luminance3& retValue, const AtmosphereParameters& atmosphere,
	Position camera, Direction view_ray, Length shadow_length,
	Direction sun_direction, DimensionlessSpectrum& transmittance) {
	return UnifyShaderCode(
		GetSkyRadiance(retValue, atmosphere, transmittance_texture,
			scattering_texture, single_mie_scattering_texture,
			camera, view_ray, shadow_length, sun_direction, transmittance);
	retValue = retValue * SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
	);
}

String SkyShadingFS::GetSkyLuminanceToPointShading(Luminance3& retValue, const AtmosphereParameters& atmosphere,
	Position camera, Position point, Length shadow_length,
	Direction sun_direction, DimensionlessSpectrum& transmittance) {
	return UnifyShaderCode(
		GetSkyRadianceToPoint(retValue, atmosphere, transmittance_texture,
			scattering_texture, single_mie_scattering_texture,
			camera, point, shadow_length, sun_direction, transmittance);
		retValue = retValue * SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
	);
}

String SkyShadingFS::GetSunAndSkyIlluminance(Illuminance3& retValue, const AtmosphereParameters& atmosphere, Position p, Direction normal, Direction sun_direction,
	IrradianceSpectrum& sky_irradiance) {
	return UnifyShaderCode(
		IrradianceSpectrum sun_irradiance;
		GetSunAndSkyIrradiance(sun_irradiance,
			atmosphere, transmittance_texture, irradiance_texture, p, normal,
			sun_direction, sky_irradiance);
			sky_irradiance *= SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
		retValue = sun_irradiance * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
	);
}

String SkyShadingFS::GetShaderText() {
	return UnifyShaderCode(
		outputColor = float4(1.0, 1.0, 1.0, 1.0);
	);
}

TObject<IReflect>& SkyShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transmittance_texture);
		ReflectProperty(scattering_texture);
		ReflectProperty(single_mie_scattering_texture);
		ReflectProperty(irradiance_texture);

		ReflectProperty(SKY_SPECTRAL_RADIANCE_TO_LUMINANCE)[paramBuffer];
		ReflectProperty(SUN_SPECTRAL_RADIANCE_TO_LUMINANCE)[paramBuffer];

		ReflectProperty(worldPosition)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[BindOutput(BindOutput::COLOR)];
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(GetSolarRadiance)[MetaParameter("retValue")][MetaParameter("atmosphere")];
		ReflectMethod(GetSkyRadianceShading)[MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("camera")][MetaParameter("view_ray")][MetaParameter("shadow_length")][MetaParameter("sun_direction")][MetaParameter("transmittance")];
		ReflectMethod(GetSkyRadianceToPointShading)[MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("camera")][MetaParameter("point")][MetaParameter("shadow_length")][MetaParameter("sun_direction")][MetaParameter("transmittance")];
		ReflectMethod(GetSunAndSkyIrradianceShading)[MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("p")][MetaParameter("normal")][MetaParameter("sun_direction")][MetaParameter("sky_irradiance")];
		ReflectMethod(GetSolarLuminance)[MetaParameter("retValue")][MetaParameter("atmosphere")];
		ReflectMethod(GetSkyLuminanceShading)[MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("camera")][MetaParameter("view_ray")][MetaParameter("shadow_length")][MetaParameter("sun_direction")][MetaParameter("transmittance")];
		ReflectMethod(GetSkyLuminanceToPointShading)[MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("camera")][MetaParameter("point")][MetaParameter("shadow_length")][MetaParameter("sun_direction")][MetaParameter("transmittance")];
		ReflectMethod(GetSunAndSkyIlluminance)[MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("p")][MetaParameter("normal")][MetaParameter("sun_direction")][MetaParameter("sky_irradiance")];
	}

	return *this;
}