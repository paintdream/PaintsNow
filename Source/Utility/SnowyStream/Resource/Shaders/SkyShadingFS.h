// SkyShadingFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "SkyCommonFS.h"

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
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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

	class SkyShadingFS : public TReflected<SkyShadingFS, SkyCommonFS> {
	public:
		SkyShadingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		String GetSolarRadiance(RadianceSpectrum& retValue, const AtmosphereParameters& atmosphere);
		String GetSkyRadianceShading(RadianceSpectrum& retValue, const AtmosphereParameters& atmosphere,
			Position camera, Direction view_ray, Length shadow_length,
			Direction sun_direction, DimensionlessSpectrum& transmittance);
		String GetSkyRadianceToPointShading(RadianceSpectrum& retValue, const AtmosphereParameters& atmosphere,
			Position camera, Position point, Length shadow_length,
			Direction sun_direction, DimensionlessSpectrum& transmittance);
		String GetSunAndSkyIrradianceShading(IrradianceSpectrum& retValue, const AtmosphereParameters& atmosphere,
			Position p, Direction normal, Direction sun_direction,
			IrradianceSpectrum& sky_irradiance);
		String GetSolarLuminance(Luminance3& retValue, const AtmosphereParameters& atmosphere);
		String GetSkyLuminanceShading(Luminance3& retValue, const AtmosphereParameters& atmosphere, Position camera, Direction view_ray, Length shadow_length,
			Direction sun_direction, DimensionlessSpectrum& transmittance);
		String GetSkyLuminanceToPointShading(Luminance3& retValue, const AtmosphereParameters& atmosphere,
			Position camera, Position point, Length shadow_length,
			Direction sun_direction, DimensionlessSpectrum& transmittance);
		String GetSunAndSkyIlluminance(Illuminance3& retValue, const AtmosphereParameters& atmosphere,
			Position p, Direction normal, Direction sun_direction,
			IrradianceSpectrum& sky_irradiance);

		TransmittanceTexture transmittance_texture;
		ScatteringTexture scattering_texture;
		ReducedScatteringTexture single_mie_scattering_texture;
		IrradianceTexture irradiance_texture;
		
		// Uniforms
		Float3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
		Float3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;

		// Input
		Float3 worldPosition;

		// Output
		Float4 outputColor;
	};
}

