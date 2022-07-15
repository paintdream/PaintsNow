#include "SkyComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "../Transform/TransformComponent.h"
#include <utility>

using namespace PaintsNow;

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

static const int kLambdaMin = 360;
static const int kLambdaMax = 830;
// Values from "CIE (1931) 2-deg color matching functions", see
// "http://web.archive.org/web/20081228084047/
//	http://www.cvrl.org/database/data/cmfs/ciexyz31.txt".
static const double CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[380] = {
	360, 0.000129900000, 0.000003917000, 0.000606100000,
	365, 0.000232100000, 0.000006965000, 0.001086000000,
	370, 0.000414900000, 0.000012390000, 0.001946000000,
	375, 0.000741600000, 0.000022020000, 0.003486000000,
	380, 0.001368000000, 0.000039000000, 0.006450001000,
	385, 0.002236000000, 0.000064000000, 0.010549990000,
	390, 0.004243000000, 0.000120000000, 0.020050010000,
	395, 0.007650000000, 0.000217000000, 0.036210000000,
	400, 0.014310000000, 0.000396000000, 0.067850010000,
	405, 0.023190000000, 0.000640000000, 0.110200000000,
	410, 0.043510000000, 0.001210000000, 0.207400000000,
	415, 0.077630000000, 0.002180000000, 0.371300000000,
	420, 0.134380000000, 0.004000000000, 0.645600000000,
	425, 0.214770000000, 0.007300000000, 1.039050100000,
	430, 0.283900000000, 0.011600000000, 1.385600000000,
	435, 0.328500000000, 0.016840000000, 1.622960000000,
	440, 0.348280000000, 0.023000000000, 1.747060000000,
	445, 0.348060000000, 0.029800000000, 1.782600000000,
	450, 0.336200000000, 0.038000000000, 1.772110000000,
	455, 0.318700000000, 0.048000000000, 1.744100000000,
	460, 0.290800000000, 0.060000000000, 1.669200000000,
	465, 0.251100000000, 0.073900000000, 1.528100000000,
	470, 0.195360000000, 0.090980000000, 1.287640000000,
	475, 0.142100000000, 0.112600000000, 1.041900000000,
	480, 0.095640000000, 0.139020000000, 0.812950100000,
	485, 0.057950010000, 0.169300000000, 0.616200000000,
	490, 0.032010000000, 0.208020000000, 0.465180000000,
	495, 0.014700000000, 0.258600000000, 0.353300000000,
	500, 0.004900000000, 0.323000000000, 0.272000000000,
	505, 0.002400000000, 0.407300000000, 0.212300000000,
	510, 0.009300000000, 0.503000000000, 0.158200000000,
	515, 0.029100000000, 0.608200000000, 0.111700000000,
	520, 0.063270000000, 0.710000000000, 0.078249990000,
	525, 0.109600000000, 0.793200000000, 0.057250010000,
	530, 0.165500000000, 0.862000000000, 0.042160000000,
	535, 0.225749900000, 0.914850100000, 0.029840000000,
	540, 0.290400000000, 0.954000000000, 0.020300000000,
	545, 0.359700000000, 0.980300000000, 0.013400000000,
	550, 0.433449900000, 0.994950100000, 0.008749999000,
	555, 0.512050100000, 1.000000000000, 0.005749999000,
	560, 0.594500000000, 0.995000000000, 0.003900000000,
	565, 0.678400000000, 0.978600000000, 0.002749999000,
	570, 0.762100000000, 0.952000000000, 0.002100000000,
	575, 0.842500000000, 0.915400000000, 0.001800000000,
	580, 0.916300000000, 0.870000000000, 0.001650001000,
	585, 0.978600000000, 0.816300000000, 0.001400000000,
	590, 1.026300000000, 0.757000000000, 0.001100000000,
	595, 1.056700000000, 0.694900000000, 0.001000000000,
	600, 1.062200000000, 0.631000000000, 0.000800000000,
	605, 1.045600000000, 0.566800000000, 0.000600000000,
	610, 1.002600000000, 0.503000000000, 0.000340000000,
	615, 0.938400000000, 0.441200000000, 0.000240000000,
	620, 0.854449900000, 0.381000000000, 0.000190000000,
	625, 0.751400000000, 0.321000000000, 0.000100000000,
	630, 0.642400000000, 0.265000000000, 0.000049999990,
	635, 0.541900000000, 0.217000000000, 0.000030000000,
	640, 0.447900000000, 0.175000000000, 0.000020000000,
	645, 0.360800000000, 0.138200000000, 0.000010000000,
	650, 0.283500000000, 0.107000000000, 0.000000000000,
	655, 0.218700000000, 0.081600000000, 0.000000000000,
	660, 0.164900000000, 0.061000000000, 0.000000000000,
	665, 0.121200000000, 0.044580000000, 0.000000000000,
	670, 0.087400000000, 0.032000000000, 0.000000000000,
	675, 0.063600000000, 0.023200000000, 0.000000000000,
	680, 0.046770000000, 0.017000000000, 0.000000000000,
	685, 0.032900000000, 0.011920000000, 0.000000000000,
	690, 0.022700000000, 0.008210000000, 0.000000000000,
	695, 0.015840000000, 0.005723000000, 0.000000000000,
	700, 0.011359160000, 0.004102000000, 0.000000000000,
	705, 0.008110916000, 0.002929000000, 0.000000000000,
	710, 0.005790346000, 0.002091000000, 0.000000000000,
	715, 0.004109457000, 0.001484000000, 0.000000000000,
	720, 0.002899327000, 0.001047000000, 0.000000000000,
	725, 0.002049190000, 0.000740000000, 0.000000000000,
	730, 0.001439971000, 0.000520000000, 0.000000000000,
	735, 0.000999949300, 0.000361100000, 0.000000000000,
	740, 0.000690078600, 0.000249200000, 0.000000000000,
	745, 0.000476021300, 0.000171900000, 0.000000000000,
	750, 0.000332301100, 0.000120000000, 0.000000000000,
	755, 0.000234826100, 0.000084800000, 0.000000000000,
	760, 0.000166150500, 0.000060000000, 0.000000000000,
	765, 0.000117413000, 0.000042400000, 0.000000000000,
	770, 0.000083075270, 0.000030000000, 0.000000000000,
	775, 0.000058706520, 0.000021200000, 0.000000000000,
	780, 0.000041509940, 0.000014990000, 0.000000000000,
	785, 0.000029353260, 0.000010600000, 0.000000000000,
	790, 0.000020673830, 0.000007465700, 0.000000000000,
	795, 0.000014559770, 0.000005257800, 0.000000000000,
	800, 0.000010253980, 0.000003702900, 0.000000000000,
	805, 0.000007221456, 0.000002607800, 0.000000000000,
	810, 0.000005085868, 0.000001836600, 0.000000000000,
	815, 0.000003581652, 0.000001293400, 0.000000000000,
	820, 0.000002522525, 0.000000910930, 0.000000000000,
	825, 0.000001776509, 0.000000641530, 0.000000000000,
	830, 0.000001251141, 0.000000451810, 0.000000000000,
};

// The conversion matrix from XYZ to linear sRGB color spaces.
// Values from https://en.wikipedia.org/wiki/SRGB.
static const double XYZ_TO_SRGB[9] = {
	+3.2406, -1.5372, -0.4986,
	-0.9689, +1.8758, +0.0415,
	+0.0557, -0.2040, +1.0570
};

static double CieColorMatchingFunctionTableValue(double wavelength, int column) {
	if (wavelength <= kLambdaMin || wavelength >= kLambdaMax) {
		return 0.0;
	}
	double u = (wavelength - kLambdaMin) / 5.0;
	int row = static_cast<int>(floor(u));
	assert(row >= 0 && row + 1 < 95);
	assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
	u -= row;
	return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

static double Interpolate(
	const std::vector<double>& wavelengths,
	const std::vector<double>& wavelength_function,
	double wavelength) {
	assert(wavelength_function.size() == wavelengths.size());
	if (wavelength < wavelengths[0]) {
		return wavelength_function[0];
	}
	for (unsigned int i = 0; i < wavelengths.size() - 1; ++i) {
		if (wavelength < wavelengths[i + 1]) {
			double u =
				(wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
			return
				wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
		}
	}
	return wavelength_function[wavelength_function.size() - 1];
}

/*
<p>We can then implement a utility function to compute the "spectral radiance to
luminance" conversion constants (see Section 14.3 in <a
href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
Evaluation of 8 Clear Sky Models</a> for their definitions):
*/

static const double kLambdaR = 680.0;
static const double kLambdaG = 550.0;
static const double kLambdaB = 440.0;
static const double MAX_LUMINOUS_EFFICACY = 683.0;

// The returned constants are in lumen.nm / watt.
static void ComputeSpectralRadianceToLuminanceFactors(
	const std::vector<double>& wavelengths,
	const std::vector<double>& solar_irradiance,
	double lambda_power, double* k_r, double* k_g, double* k_b) {
	*k_r = 0.0;
	*k_g = 0.0;
	*k_b = 0.0;
	double solar_r = Interpolate(wavelengths, solar_irradiance, kLambdaR);
	double solar_g = Interpolate(wavelengths, solar_irradiance, kLambdaG);
	double solar_b = Interpolate(wavelengths, solar_irradiance, kLambdaB);
	int dlambda = 1;
	for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
		double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
		double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
		double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
		const double* xyz2srgb = XYZ_TO_SRGB;
		double r_bar =
			xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
		double g_bar =
			xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
		double b_bar =
			xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
		double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
		*k_r += r_bar * irradiance / solar_r *
			pow(lambda / kLambdaR, lambda_power);
		*k_g += g_bar * irradiance / solar_g *
			pow(lambda / kLambdaG, lambda_power);
		*k_b += b_bar * irradiance / solar_b *
			pow(lambda / kLambdaB, lambda_power);
	}
	*k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}

SkyComponent::GlobalParameters::GlobalParameters() {
	TRANSMITTANCE_TEXTURE_WIDTH = 256;
	TRANSMITTANCE_TEXTURE_HEIGHT = 64;

	SCATTERING_TEXTURE_R_SIZE = 32;
	SCATTERING_TEXTURE_MU_SIZE = 128;
	SCATTERING_TEXTURE_MU_S_SIZE = 32;
	SCATTERING_TEXTURE_NU_SIZE = 8;

	SCATTERING_TEXTURE_WIDTH = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
	SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
	SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

	IRRADIANCE_TEXTURE_WIDTH = 64;
	IRRADIANCE_TEXTURE_HEIGHT = 16;

	MAX_LUMINOUS_EFFICACY = 683.0;
}

TObject<IReflect>& SkyComponent::GlobalParameters::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(TRANSMITTANCE_TEXTURE_WIDTH);
		ReflectProperty(TRANSMITTANCE_TEXTURE_HEIGHT);

		ReflectProperty(SCATTERING_TEXTURE_R_SIZE);
		ReflectProperty(SCATTERING_TEXTURE_MU_SIZE);
		ReflectProperty(SCATTERING_TEXTURE_MU_S_SIZE);
		ReflectProperty(SCATTERING_TEXTURE_NU_SIZE);

		ReflectProperty(SCATTERING_TEXTURE_WIDTH);
		ReflectProperty(SCATTERING_TEXTURE_HEIGHT);
		ReflectProperty(SCATTERING_TEXTURE_DEPTH);

		ReflectProperty(IRRADIANCE_TEXTURE_WIDTH);
		ReflectProperty(IRRADIANCE_TEXTURE_HEIGHT);

		ReflectProperty(MAX_LUMINOUS_EFFICACY);
	}

	return *this;
}

SkyComponent::SkyComponent(const TShared<MeshResource>& res, const TShared<BatchComponent>& batch) : BaseClass(res, batch) {
	assert(res);
	Flag().fetch_or(COMPONENT_SHARED, std::memory_order_relaxed); // can be shared among different entities

	const double kPi = 3.1415926;
	const double kSunAngularRadius = 0.00935 / 2.0;
	const double kSunSolidAngle = kPi * kSunAngularRadius * kSunAngularRadius;
	const double kLengthUnitInMeters = 1000.0;

	// Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
	// (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
	// summed and averaged in each bin (e.g. the value for 360nm is the average
	// of the ASTM G-173 values for all wavelengths between 360 and 370nm).
	// Values in W.m^-2.
	const bool use_half_precision_ = false;
	const bool use_constant_solar_spectrum_ = true;
	const bool use_ozone_ = true;

	const int kLambdaMin = 360;
	const int kLambdaMax = 830;
	const double kSolarIrradiance[48] = {
		1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
		1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
		1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
		1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
		1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
		1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992
	};
	// Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
	// referencespectra/o3spectra2011/index.html for 233K, summed and averaged in
	// each bin (e.g. the value for 360nm is the average of the original values
	// for all wavelengths between 360 and 370nm). Values in m^2.
	const double kOzoneCrossSection[48] = {
		1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
		8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
		1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
		4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
		2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
		6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
		2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
	};
	// From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
	const double kDobsonUnit = 2.687e20;
	// Maximum number density of ozone molecules, in m^-3 (computed so at to get
	// 300 Dobson units of ozone - for this we divide 300 DU by the integral of
	// the ozone density profile defined below, which is equal to 15km).
	const double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
	// Wavelength independent solar irradiance "spectrum" (not physically
	// realistic, but was used in the original implementation).
	const double kConstantSolarIrradiance = 1.5;
	const double kBottomRadius = 6360000.0;
	const double kTopRadius = 6420000.0;
	const double kRayleigh = 1.24062e-6;
	const double kRayleighScaleHeight = 8000.0;
	const double kMieScaleHeight = 1200.0;
	const double kMieAngstromAlpha = 0.0;
	const double kMieAngstromBeta = 5.328e-3;
	const double kMieSingleScatteringAlbedo = 0.9;
	const double kMiePhaseFunctionG = 0.8;
	const double kGroundAlbedo = 0.1;
	const double max_sun_zenith_angle =
		(use_half_precision_ ? 102.0 : 120.0) / 180.0 * kPi;

	DensityProfileLayer
		rayleigh_layer(0.0, 1.0, -1.0 / kRayleighScaleHeight, 0.0, 0.0);
	DensityProfileLayer mie_layer(0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0);
	// Density profile increasing linearly from 0 to 1 between 10 and 25km, and
	// decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
	// profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
	// Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
	std::vector<DensityProfileLayer> ozone_density;
	ozone_density.push_back(
		DensityProfileLayer(25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
	ozone_density.push_back(
		DensityProfileLayer(0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));

	std::vector<double> wavelengths;
	std::vector<double> solar_irradiance;
	std::vector<double> rayleigh_scattering;
	std::vector<double> mie_scattering;
	std::vector<double> mie_extinction;
	std::vector<double> absorption_extinction;
	std::vector<double> ground_albedo;
	for (int l = kLambdaMin; l <= kLambdaMax; l += 10) {
		double lambda = static_cast<double>(l) * 1e-3;	// micro-meters
		double mie =
			kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
		wavelengths.push_back(l);
		if (use_constant_solar_spectrum_) {
			solar_irradiance.push_back(kConstantSolarIrradiance);
		} else {
			solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
		}
		rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
		mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
		mie_extinction.push_back(mie);
		absorption_extinction.push_back(use_ozone_ ?
			kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10] :
			0.0);
		ground_albedo.push_back(kGroundAlbedo);
	}

	std::vector<DensityProfileLayer> rayleigh_layer_vector;
	rayleigh_layer_vector.emplace_back(rayleigh_layer);
	std::vector<DensityProfileLayer> mie_layer_vector;
	mie_layer_vector.emplace_back(mie_layer);
	InitializeAtmosphereParameters(wavelengths, solar_irradiance, kSunAngularRadius,
		kBottomRadius, kTopRadius, rayleigh_layer_vector, rayleigh_scattering,
		mie_layer_vector, mie_scattering, mie_extinction, kMiePhaseFunctionG,
		ozone_density, absorption_extinction, ground_albedo, max_sun_zenith_angle,
		kLengthUnitInMeters, 15);
}

void SkyComponent::UpdateMaterial() {

}

static Float3 ToVector3(const std::vector<double>& wavelengths, const std::vector<double>& v, const Float3& lambdas, double scale) {
	assert(v.size() >= 3);
	double r = Interpolate(wavelengths, v, lambdas[0]) * scale;
	double g = Interpolate(wavelengths, v, lambdas[1]) * scale;
	double b = Interpolate(wavelengths, v, lambdas[2]) * scale;
	return Float3((float)r, (float)g, (float)b);
}

static void SetDensityProfile(DensityProfile& profile, const std::vector<DensityProfileLayer>& layers) {
	if (layers.size() > 1) {
		profile.layer_bottom = layers[0];
		profile.layer_top = layers[1];
	} else {
		assert(layers.size() != 0);
		profile.layer_top = layers[0];
	}
}

void SkyComponent::InitializeAtmosphereParameters(const std::vector<double>& wavelengths,
	const std::vector<double>& solar_irradiance,
	const double sun_angular_radius,
	double bottom_radius,
	double top_radius,
	const std::vector<DensityProfileLayer>& rayleigh_density,
	const std::vector<double>& rayleigh_scattering,
	const std::vector<DensityProfileLayer>& mie_density,
	const std::vector<double>& mie_scattering,
	const std::vector<double>& mie_extinction,
	double mie_phase_function_g,
	const std::vector<DensityProfileLayer>& absorption_density,
	const std::vector<double>& absorption_extinction,
	const std::vector<double>& ground_albedo,
	double max_sun_zenith_angle,
	double length_unit_in_meters,
	unsigned int num_precomputed_wavelengths) {

	bool precompute_illuminance = num_precomputed_wavelengths > 3;
	double sky_k_r, sky_k_g, sky_k_b;
	if (precompute_illuminance) {
		sky_k_r = sky_k_g = sky_k_b = MAX_LUMINOUS_EFFICACY;
	} else {
		ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance,
			-3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
	}
	// Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
	double sun_k_r, sun_k_g, sun_k_b;
	ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance,
		0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);

	AtmosphereParameters& p = atmosphereParameters;
	Float3 lambdas((float)kLambdaR, (float)kLambdaG, (float)kLambdaB);

	p.solar_irradiance = ToVector3(wavelengths, solar_irradiance, lambdas, 1.0);
	p.sun_angular_radius = (float)sun_angular_radius;
	p.bottom_radius = (float)(bottom_radius / length_unit_in_meters);
	p.top_radius = (float)(top_radius / length_unit_in_meters);
	SetDensityProfile(p.rayleigh_density, rayleigh_density);
	p.rayleigh_scattering = ToVector3(wavelengths, rayleigh_scattering, lambdas, length_unit_in_meters);
	SetDensityProfile(p.mie_density, mie_density);
	p.mie_scattering = ToVector3(wavelengths, mie_scattering, lambdas, length_unit_in_meters);
	p.mie_extinction = ToVector3(wavelengths, mie_extinction, lambdas, length_unit_in_meters);
	p.mie_phase_function_g = (float)mie_phase_function_g;
	SetDensityProfile(p.absorption_density, absorption_density);
	p.absorption_extinction = ToVector3(wavelengths, absorption_extinction, lambdas, length_unit_in_meters);
	p.ground_albedo = ToVector3(wavelengths, ground_albedo, lambdas, length_unit_in_meters);
	p.mu_s_min = (float)cos(max_sun_zenith_angle);

	GlobalParameters& g = globalParameters;
	g.SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = Float3((float)sky_k_r, (float)sky_k_g, (float)sky_k_b);
	g.SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = Float3((float)sun_k_r, (float)sun_k_g, (float)sun_k_b);
}

TObject<IReflect>& SkyComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(atmosphereParameters);
		ReflectProperty(transmittance_texture)[Runtime];
		ReflectProperty(scattering_texture)[Runtime];
		ReflectProperty(irradiance_texture)[Runtime];
	}

	return *this;
}

static TShared<TextureResource> NewTexture2D(SnowyStream& snowyStream, const String& path, int width, int height) {
	TShared<TextureResource> textureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), path, false, ResourceBase::RESOURCE_VIRTUAL);
	IRender::Queue* renderQueue = snowyStream.GetRenderResourceManager()->GetWarpResourceQueue();

	IRender::Resource::TextureDescription& description = textureResource->description;
	description.dimension = UShort3(verify_cast<uint16_t>(width), verify_cast<uint16_t>(height), 1);
	description.state.format = IRender::Resource::TextureDescription::FLOAT;
	description.state.layout = IRender::Resource::TextureDescription::RGBA;
	description.state.sample = IRender::Resource::TextureDescription::LINEAR;
	description.state.addressU = description.state.addressV = description.state.addressW = IRender::Resource::TextureDescription::CLAMP;
	textureResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
	textureResource->GetResourceManager().InvokeUpload(textureResource(), renderQueue);

	return textureResource;
}

static TShared<TextureResource> NewTexture3D(SnowyStream& snowyStream, const String& path, int width, int height, int depth, bool half) {
	TShared<TextureResource> textureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), path, false, ResourceBase::RESOURCE_VIRTUAL);
	IRender::Queue* renderQueue = snowyStream.GetRenderResourceManager()->GetWarpResourceQueue();

	IRender::Resource::TextureDescription& description = textureResource->description;
	description.dimension = UShort3(verify_cast<uint16_t>(width), verify_cast<uint16_t>(height), verify_cast<uint16_t>(depth));
	description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	description.state.format = half ? IRender::Resource::TextureDescription::HALF : IRender::Resource::TextureDescription::FLOAT;
	description.state.layout = IRender::Resource::TextureDescription::RGBA;
	description.state.sample = IRender::Resource::TextureDescription::LINEAR;
	description.state.addressU = description.state.addressV = description.state.addressW = IRender::Resource::TextureDescription::CLAMP;
	textureResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
	textureResource->GetResourceManager().InvokeUpload(textureResource(), renderQueue);

	return textureResource;
}

void SkyComponent::Initialize(Engine& engine, Entity* entity) {
	SnowyStream& snowyStream = engine.snowyStream;
	transmittance_texture = NewTexture2D(snowyStream, ResourceBase::GenerateLocation("SkyTransmittance", this), globalParameters.TRANSMITTANCE_TEXTURE_WIDTH, globalParameters.TRANSMITTANCE_TEXTURE_HEIGHT);
	scattering_texture = NewTexture3D(snowyStream, ResourceBase::GenerateLocation("SkyScattering", this), globalParameters.SCATTERING_TEXTURE_WIDTH, globalParameters.SCATTERING_TEXTURE_HEIGHT, globalParameters.SCATTERING_TEXTURE_DEPTH, true);
	irradiance_texture = NewTexture2D(snowyStream, ResourceBase::GenerateLocation("SkyIrradiance", this), globalParameters.IRRADIANCE_TEXTURE_WIDTH, globalParameters.IRRADIANCE_TEXTURE_HEIGHT);

	BaseClass::Initialize(engine, entity);
}

void SkyComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
}

size_t SkyComponent::ReportGraphicMemoryUsage() const {
	return BaseClass::ReportGraphicMemoryUsage();
}

void SkyComponent::SetSkyTexture(const TShared<TextureResource>& texture) {
	skyTexture = texture;
}

uint32_t SkyComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) {
	// override is not allowed
	if (inputRenderData.overrideShaderTemplate != nullptr)
		return 0;

	if (!skyTexture)
		return 0;

	uint32_t start = verify_cast<uint32_t>(outputDrawCalls.size());
	uint32_t count = BaseClass::CollectDrawCalls(outputDrawCalls, inputRenderData, bytesCache, option);
	if (count == ~(uint32_t)0) return count;

	for (uint32_t i = start; i < count; i++) {
		OutputRenderData& renderData = outputDrawCalls[i];
		PassBase::Updater& updater = renderData.shaderResource->GetPassUpdater();
		IRender::Resource::QuickDrawCallDescription& drawCall = renderData.drawCallDescription;
		IRender::Resource::RenderStateDescription& renderState = renderData.renderStateDescription;
		renderState.stencilReplacePass = 0;
		renderState.cull = 1;
		renderState.cullFrontFace = 1;
		renderState.fill = 1;
		renderState.colorWrite = 1;
		renderState.blend = 0;
		renderState.depthTest = IRender::Resource::RenderStateDescription::GREATER;
		renderState.depthWrite = 0;
		renderState.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		renderState.stencilWrite = 0;
		renderState.stencilMask = 0;
		renderState.stencilValue = 0;
		const PassBase::Parameter& paramMainTexture = updater[IShader::BindInput::MAINTEXTURE];
		drawCall.GetTextures()[paramMainTexture.slot] = skyTexture()->GetRenderResource();
	}

	return verify_cast<uint32_t>(outputDrawCalls.size()) - start;
}
