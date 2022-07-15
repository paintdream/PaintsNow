// SkyCommonFS.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-3
//

#pragma once
#include "../../../../General/Interface/IShader.h"
#include "SkyCommonDef.h"

namespace PaintsNow {
	// [Precomputed Atmospheric Scattering](https://hal.inria.fr/inria-00288758/en)
// https://ebruneton.github.io/precomputed_atmospheric_scattering/demo.html
/**
* Copyright (c) 2017 Eric Bruneton
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holders nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

	class pure_interface SkyCommonFS : public TReflected<SkyCommonFS, IShader> {
	public:
		SkyCommonFS();
		String GetPredefines() override;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		IShader::BindBuffer paramBuffer;
		String DistanceToTopAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu);
		String DistanceToBottomAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu);
		String RayIntersectsGround(bool& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu);
		String GetLayerDensity(Number& retValue, const DensityProfileLayer& layer, Length altitude);
		String GetProfileDensity(Number& retValue, const DensityProfile& profile, Length altitude);
		String ComputeOpticalLengthToTopAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, const DensityProfile& profile, Length r, Number mu);
		String ComputeTransmittanceToTopAtmosphereBoundary(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu);
		String GetTextureCoordFromUnitRange(Number& retValue, Number x, float texture_size);
		String GetUnitRangeFromTextureCoord(Number& retValue, Number u, float texture_size);
		String GetTransmittanceTextureUvFromRMu(Float2& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu);
		String GetRMuFromTransmittanceTextureUv(const AtmosphereParameters& atmosphere, const Float2& uv, Length& r, Number& mu);
		String ComputeTransmittanceToTopAtmosphereBoundaryTexture(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const Float2& frag_coord);
		String GetTransmittanceToTopAtmosphereBoundary(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const TransmittanceTexture& transmittance_texture, Length r, Number mu);
		String GetTransmittance(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const TransmittanceTexture& transmittance_texture, Length r, Number mu, Length d, bool ray_r_mu_intersects_ground);
		String GetTransmittanceToSun(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const TransmittanceTexture& transmittance_texture, Length r, Number mu_s);
		String ComputeSingleScatteringIntegrand(
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			Length r, Number mu, Number mu_s, Number nu, Length d,
			bool ray_r_mu_intersects_ground,
			DimensionlessSpectrum& rayleigh, DimensionlessSpectrum& mie);
		String DistanceToNearestAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu, bool ray_r_mu_intersects_ground);
		String ComputeSingleScattering(
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			Length r, Number mu, Number mu_s, Number nu,
			bool ray_r_mu_intersects_ground,
			IrradianceSpectrum& rayleigh, IrradianceSpectrum& mie);
		String RayleighPhaseFunction(InverseSolidAngle& retValue, Number nu);
		String MiePhaseFunction(InverseSolidAngle& retValue, Number g, Number nu);
		String GetScatteringTextureUvwzFromRMuMuSNu(Float4& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu, Number mu_s, Number nu, bool ray_r_mu_intersects_ground);
		String GetRMuMuSNuFromScatteringTextureUvwz(const AtmosphereParameters& atmosphere,
			const Float4& uvwz, Length& r, Number& mu, Number& mu_s,
			Number& nu, bool& ray_r_mu_intersects_ground);
		String GetRMuMuSNuFromScatteringTextureFragCoord(
			const AtmosphereParameters& atmosphere, const Float3& frag_coord,
			Length& r, Number& mu, Number& mu_s, Number& nu,
			bool& ray_r_mu_intersects_ground);
		String ComputeSingleScatteringTexture(const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture, const Float3& frag_coord,
			IrradianceSpectrum& rayleigh, IrradianceSpectrum& mie);
		String GetScatteringAbstract(AbstractSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const AbstractSpectrumTexture& scattering_texture,
			Length r, Number mu, Number mu_s, Number nu,
			bool ray_r_mu_intersects_ground);
		String GetScatteringReduced(RadianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const ReducedScatteringTexture& single_rayleigh_scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			const ScatteringTexture& multiple_scattering_texture,
			Length r, Number mu, Number mu_s, Number nu,
			bool ray_r_mu_intersects_ground,
			int scattering_order);
		String GetIrradiance(IrradianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const IrradianceTexture& irradiance_texture,
			Length r, Number mu_s);
		String ComputeScatteringDensity(RadianceDensitySpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const ReducedScatteringTexture& single_rayleigh_scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			const ScatteringTexture& multiple_scattering_texture,
			const IrradianceTexture& irradiance_texture,
			Length r, Number mu, Number mu_s, Number nu, int scattering_order);
		String ComputeMultipleScattering(RadianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const ScatteringDensityTexture& scattering_density_texture,
			Length r, Number mu, Number mu_s, Number nu,
			bool ray_r_mu_intersects_ground);
		String ComputeScatteringDensityTexture(RadianceDensitySpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const ReducedScatteringTexture& single_rayleigh_scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			const ScatteringTexture& multiple_scattering_texture,
			const IrradianceTexture& irradiance_texture,
			const Float3& frag_coord, int scattering_order);
		String ComputeMultipleScatteringTexture(RadianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const ScatteringDensityTexture& scattering_density_texture,
			const Float3& frag_coord, Number& nu);
		String ComputeDirectIrradiance(IrradianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			Length r, Number mu_s);
		String ComputeIndirectIrradiance(IrradianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const ReducedScatteringTexture& single_rayleigh_scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			const ScatteringTexture& multiple_scattering_texture,
			Length r, Number mu_s, int scattering_order);
		String GetIrradianceTextureUvFromRMuS(Float2& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu_s);
		String GetRMuSFromIrradianceTextureUv(const AtmosphereParameters& atmosphere,
			const Float2& uv, Length& r, Number& mu_s);
		String ComputeDirectIrradianceTexture(IrradianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const Float2& frag_coord);
		String ComputeIndirectIrradianceTexture(IrradianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const ReducedScatteringTexture& single_rayleigh_scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			const ScatteringTexture& multiple_scattering_texture,
			const Float2& frag_coord, int scattering_order);
		String GetCombinedScattering(IrradianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const ReducedScatteringTexture& scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			Length r, Number mu, Number mu_s, Number nu,
			bool ray_r_mu_intersects_ground,
			IrradianceSpectrum& single_mie_scattering);
		String GetSkyRadiance(RadianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const ReducedScatteringTexture& scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			Position& camera, const Direction& view_ray, Length shadow_length,
			const Direction& sun_direction, DimensionlessSpectrum& transmittance);
		String GetSkyRadianceToPoint(RadianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const ReducedScatteringTexture& scattering_texture,
			const ReducedScatteringTexture& single_mie_scattering_texture,
			Position& camera, const Position& point, Length shadow_length,
			const Direction& sun_direction, DimensionlessSpectrum& transmittance);
		String GetSunAndSkyIrradiance(IrradianceSpectrum& retValue,
			const AtmosphereParameters& atmosphere,
			const TransmittanceTexture& transmittance_texture,
			const IrradianceTexture& irradiance_texture,
			const Position& point, const Direction& normal, const Direction& sun_direction,
			IrradianceSpectrum& sky_irradiance);

		// constants
		float TRANSMITTANCE_TEXTURE_WIDTH;
		float TRANSMITTANCE_TEXTURE_HEIGHT;
		float SCATTERING_TEXTURE_R_SIZE;
		float SCATTERING_TEXTURE_MU_SIZE;
		float SCATTERING_TEXTURE_MU_S_SIZE;
		float SCATTERING_TEXTURE_NU_SIZE;
		float SCATTERING_TEXTURE_WIDTH;
		float SCATTERING_TEXTURE_HEIGHT;
		float SCATTERING_TEXTURE_DEPTH;
		float IRRADIANCE_TEXTURE_WIDTH;
		float IRRADIANCE_TEXTURE_HEIGHT;

		AtmosphereParameters atmosphere;
		IShader::BindTexture protoTransmittanceTexture;
		IShader::BindTexture protoAbstractSpectrumTexture;
		IShader::BindTexture protoReducedScatteringTexture;
		IShader::BindTexture protoScatteringTexture;
		IShader::BindTexture protoIrradianceTexture;
		IShader::BindTexture protoScatteringDensityTexture;
	};
}