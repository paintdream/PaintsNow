// SkyCommonFS.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-3
//

#pragma once

#include "../../../../Core/Interface/IReflect.h"
#include "../../../../General/Interface/IShader.h"

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

#define Length float
#define Wavelength float
#define Angle float
#define SolidAngle float
#define Power float
#define LuminousPower float
#define Number float
#define InverseLength float
#define Area float
#define Volume float
#define NumberDensity float
#define Irradiance float
#define Radiance float
#define SpectralPower float
#define SpectralIrradiance float
#define SpectralRadiance float
#define SpectralRadianceDensity float
#define ScatteringCoefficient float
#define InverseSolidAngle float
#define LuminousIntensity float
#define Luminance float
#define Illuminance float
#define AbstractSpectrum Float3
#define DimensionlessSpectrum Float3
#define PowerSpectrum Float3
#define IrradianceSpectrum Float3
#define RadianceSpectrum Float3
#define RadianceDensitySpectrum Float3
#define ScatteringSpectrum Float3
#define Position Float3
#define Direction Float3
#define Luminance3 Float3
#define Illuminance3 Float3

	// An atmosphere layer of width 'width', and whose density is defined as
	//   'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
	// clamped to [0,1], and where h is the altitude.
	class DensityProfileLayer : public TReflected<DensityProfileLayer, IReflectObjectComplex> {
	public:
		DensityProfileLayer(double width = 0.0, double exp_term = 0.0, double exp_scale = 0.0, double linear_term = 0.0, double constant_term = 0.0);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		Length width;
		Number exp_term;
		InverseLength exp_scale;
		InverseLength linear_term;
		Number constant_term;
	};

	// An atmosphere density profile made of several layers on top of each other
	// (from bottom to top). The width of the last layer is ignored, i.e. it always
	// extend to the top atmosphere boundary. The profile values vary between 0
	// (null density) to 1 (maximum density).
	class DensityProfile : public TReflected<DensityProfile, IReflectObjectComplex> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;
		DensityProfileLayer layer_bottom;
		DensityProfileLayer layer_top;
	};

	class AtmosphereParameters : public TReflected<DensityProfile, IReflectObjectComplex> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// The solar irradiance at the top of the atmosphere.
		IrradianceSpectrum solar_irradiance;
		// The sun's angular radius. Warning: the implementation uses approximations
		// that are valid only if this angle is smaller than 0.1 radians.
		Angle sun_angular_radius;
		// The distance between the planet center and the bottom of the atmosphere.
		Length bottom_radius;
		// The distance between the planet center and the top of the atmosphere.
		Length top_radius;
		// The density profile of air molecules, i.e. a function from altitude to
		// dimensionless values between 0 (null density) and 1 (maximum density).
		DensityProfile rayleigh_density;
		// The scattering coefficient of air molecules at the altitude where their
		// density is maximum (usually the bottom of the atmosphere), as a function of
		// wavelength. The scattering coefficient at altitude h is equal to
		// 'rayleigh_scattering' times 'rayleigh_density' at this altitude.
		ScatteringSpectrum rayleigh_scattering;
		// The density profile of aerosols, i.e. a function from altitude to
		// dimensionless values between 0 (null density) and 1 (maximum density).
		DensityProfile mie_density;
		// The scattering coefficient of aerosols at the altitude where their density
		// is maximum (usually the bottom of the atmosphere), as a function of
		// wavelength. The scattering coefficient at altitude h is equal to
		// 'mie_scattering' times 'mie_density' at this altitude.
		ScatteringSpectrum mie_scattering;
		// The extinction coefficient of aerosols at the altitude where their density
		// is maximum (usually the bottom of the atmosphere), as a function of
		// wavelength. The extinction coefficient at altitude h is equal to
		// 'mie_extinction' times 'mie_density' at this altitude.
		ScatteringSpectrum mie_extinction;
		// The asymetry parameter for the Cornette-Shanks phase function for the
		// aerosols.
		Number mie_phase_function_g;
		// The density profile of air molecules that absorb light (e.g. ozone), i.e.
		// a function from altitude to dimensionless values between 0 (null density)
		// and 1 (maximum density).
		DensityProfile absorption_density;
		// The extinction coefficient of molecules that absorb light (e.g. ozone) at
		// the altitude where their density is maximum, as a function of wavelength.
		// The extinction coefficient at altitude h is equal to
		// 'absorption_extinction' times 'absorption_density' at this altitude.
		ScatteringSpectrum absorption_extinction;
		// The average albedo of the ground.
		DimensionlessSpectrum ground_albedo;
		// The cosine of the maximum Sun zenith angle for which atmospheric scattering
		// must be precomputed (for maximum precision, use the smallest Sun zenith
		// angle yielding negligible sky light radiance values. For instance, for the
		// Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
		Number mu_s_min;
	};

	typedef IShader::BindTexture TransmittanceTexture;
	typedef IShader::BindTexture AbstractSpectrumTexture;
	typedef IShader::BindTexture ReducedScatteringTexture;
	typedef IShader::BindTexture ScatteringTexture;
	typedef IShader::BindTexture IrradianceTexture;
	typedef IShader::BindTexture ScatteringDensityTexture;
}
