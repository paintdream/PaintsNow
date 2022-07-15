#include "AtmosphereModel.h"
#include "../../Engine.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

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
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *	contributors may be used to endorse or promote products derived from
 *	this software without specific prior written permission.
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

namespace Atmosphere {
	using namespace PaintsNow;
	
	static const double kLambdaR = 680.0;
	static const double kLambdaG = 550.0;
	static const double kLambdaB = 440.0;
	static const int TRANSMITTANCE_TEXTURE_WIDTH = 256;
	static const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

	static const int SCATTERING_TEXTURE_R_SIZE = 32;
	static const int SCATTERING_TEXTURE_MU_SIZE = 128;
	static const int SCATTERING_TEXTURE_MU_S_SIZE = 32;
	static const int SCATTERING_TEXTURE_NU_SIZE = 8;

	static const int SCATTERING_TEXTURE_WIDTH =
		SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
	static const int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
	static const int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

	static const int IRRADIANCE_TEXTURE_WIDTH = 64;
	static const int IRRADIANCE_TEXTURE_HEIGHT = 16;

	// The conversion factor between watts and lumens.
	const double MAX_LUMINOUS_EFFICACY = 683.0;

	// Values from "CIE (1931) 2-deg color matching functions", see
	// "http://web.archive.org/web/20081228084047/
	//    http://www.cvrl.org/database/data/cmfs/ciexyz31.txt".
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

	static const int kLambdaMin = 360;
	static const int kLambdaMax = 830;

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

	static TShared<TextureResource> NewTexture2d(Engine& engine, int width, int height) {
		IRender::Queue* resourceQueue = engine.snowyStream.GetRenderResourceManager()->GetWarpResourceQueue();
		TShared<TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "", false, ResourceBase::RESOURCE_VIRTUAL);
		IRender::Resource::TextureDescription::State& state = texture->description.state;
		state.addressU = state.addressV = state.addressW = IRender::Resource::TextureDescription::CLAMP;
		state.sample = IRender::Resource::TextureDescription::LINEAR;
		state.format = IRender::Resource::TextureDescription::FLOAT;
		state.layout = IRender::Resource::TextureDescription::RGBA;
		state.attachment = 1;

		UShort3& dimension = texture->description.dimension;
		dimension.x() = width;
		dimension.y() = height;

		texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
		texture->GetResourceManager().InvokeUpload(texture(), resourceQueue);

		return texture;
	}

	static TShared<TextureResource> NewTexture3d(Engine& engine, int width, int height, int depth, bool half_precision) {
		IRender::Queue* resourceQueue = engine.snowyStream.GetRenderResourceManager()->GetWarpResourceQueue();
		TShared<TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "", false, ResourceBase::RESOURCE_VIRTUAL);
		IRender::Resource::TextureDescription::State& state = texture->description.state;
		state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
		state.addressU = state.addressV = state.addressW = IRender::Resource::TextureDescription::CLAMP;
		state.sample = IRender::Resource::TextureDescription::LINEAR;
		state.format = half_precision ? IRender::Resource::TextureDescription::HALF : IRender::Resource::TextureDescription::FLOAT;
		state.layout = IRender::Resource::TextureDescription::RGBA;
		state.attachment = 1;

		UShort3& dimension = texture->description.dimension;
		dimension.x() = width;
		dimension.y() = height;
		dimension.z() = depth;

		texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
		texture->GetResourceManager().InvokeUpload(texture(), resourceQueue);

		return texture;
	}

	/*<h3 id="implementation">Model implementation</h3>

	<p>Using the above utility functions and classes, we can now implement the
	constructor of the <code>Model</code> class. This constructor generates a piece
	of GLSL code that defines an <code>ATMOSPHERE</code> constant containing the
	atmosphere parameters (we use constants instead of uniforms to enable constant
	folding and propagation optimizations in the GLSL compiler), concatenated with
	<a href="functions.glsl.html">functions.glsl</a>, and with
	<code>kAtmosphereShader</code>, to get the shader exposed by our API in
	<code>GetShader</code>. It also allocates the precomputed textures (but does not
	initialize them), as well as a vertex buffer object to render a full screen quad
	(used to render into the precomputed textures).
	*/

	Model::Model(
		Engine& engine,
		const std::vector<double>& wavelengths,
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
		unsigned int num_precomputed_wavelengths,
		bool combine_scattering_textures,
		bool half_precision) :
		num_precomputed_wavelengths_(num_precomputed_wavelengths),
		half_precision_(half_precision) {

		// Compute the values for the SKY_RADIANCE_TO_LUMINANCE constant. In theory
		// this should be 1 in precomputed illuminance mode (because the precomputed
		// textures already contain illuminance values). In practice, however, storing
		// true illuminance values in half precision textures yields artefacts
		// (because the values are too large), so we store illuminance values divided
		// by MAX_LUMINOUS_EFFICACY instead. This is why, in precomputed illuminance
		// mode, we set SKY_RADIANCE_TO_LUMINANCE to MAX_LUMINOUS_EFFICACY.
		bool precompute_illuminance = num_precomputed_wavelengths > 3;
		double sky_k_r, sky_k_g, sky_k_b;
		if (precompute_illuminance) {
			sky_k_r = sky_k_g = sky_k_b = MAX_LUMINOUS_EFFICACY;
		}
		else {
			ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance,
				-3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
		}
		// Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
		double sun_k_r, sun_k_g, sun_k_b;
		ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance,
			0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);

		// Allocate the precomputed textures, but don't precompute them yet.
		transmittance_texture_ = NewTexture2d(engine,
			TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
		scattering_texture_ = NewTexture3d(engine,
			SCATTERING_TEXTURE_WIDTH,
			SCATTERING_TEXTURE_HEIGHT,
			SCATTERING_TEXTURE_DEPTH,
			half_precision);
		if (combine_scattering_textures) {
			optional_single_mie_scattering_texture_ = 0;
		}
		else {
			optional_single_mie_scattering_texture_ = NewTexture3d(
				engine,
				SCATTERING_TEXTURE_WIDTH,
				SCATTERING_TEXTURE_HEIGHT,
				SCATTERING_TEXTURE_DEPTH,
				half_precision);
		}
		irradiance_texture_ = NewTexture2d(engine,
			IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

		// Create and compile the shader providing our API.
		/*
		std::string shader =
			glsl_header_factory_({ kLambdaR, kLambdaG, kLambdaB }) +
			(precompute_illuminance ? "" : "#define RADIANCE_API_ENABLED\n") +
			kAtmosphereShader;
		const char* source = shader.c_str();
		atmosphere_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(atmosphere_shader_, 1, &source, NULL);
		glCompileShader(atmosphere_shader_);

		// Create a full screen quad vertex array and vertex buffer objects.
		glGenVertexArrays(1, &full_screen_quad_vao_);
		glBindVertexArray(full_screen_quad_vao_);
		glGenBuffers(1, &full_screen_quad_vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, full_screen_quad_vbo_);
		const GLfloat vertices[] = {
		  -1.0, -1.0,
		  +1.0, -1.0,
		  -1.0, +1.0,
		  +1.0, +1.0,
		};
		const int kCoordsPerVertex = 2;
		glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
		const uint32_t kAttribIndex = 0;
		glVertexAttribPointer(kAttribIndex, kCoordsPerVertex, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(kAttribIndex);
		glBindVertexArray(0);
		*/
	}

	/*
	<p>The destructor is trivial:
	*/

	Model::~Model() {}

	/*
	<p>The Init method precomputes the atmosphere textures. It first allocates the
	temporary resources it needs, then calls <code>Precompute</code> to do the
	actual precomputations, and finally destroys the temporary resources.

	<p>Note that there are two precomputation modes here, depending on whether we
	want to store precomputed irradiance or illuminance values:
	<ul>
	  <li>In precomputed irradiance mode, we simply need to call
	  <code>Precompute</code> with the 3 wavelengths for which we want to precompute
	  irradiance, namely <code>kLambdaR</code>, <code>kLambdaG</code>,
	  <code>kLambdaB</code> (with the identity matrix for
	  <code>luminance_from_radiance</code>, since we don't want any conversion from
	  radiance to luminance)</li>
	  <li>In precomputed illuminance mode, we need to precompute irradiance for
	  <code>num_precomputed_wavelengths_</code>, and then integrate the results,
	  multiplied with the 3 CIE xyz color matching functions and the XYZ to sRGB
	  matrix to get sRGB illuminance values.
	  <p>A naive solution would be to allocate temporary textures for the
	  intermediate irradiance results, then perform the integration from irradiance
	  to illuminance and store the result in the final precomputed texture. In
	  pseudo-code (and assuming one wavelength per texture instead of 3):
	  <pre>
		create n temporary irradiance textures
		for each wavelength lambda in the n wavelengths:
		   precompute irradiance at lambda into one of the temporary textures
		initializes the final illuminance texture with zeros
		for each wavelength lambda in the n wavelengths:
		  accumulate in the final illuminance texture the product of the
		  precomputed irradiance at lambda (read from the temporary textures)
		  with the value of the 3 sRGB color matching functions at lambda (i.e.
		  the product of the XYZ to sRGB matrix with the CIE xyz color matching
		  functions).
	  </pre>
	  <p>However, this be would waste GPU memory. Instead, we can avoid allocating
	  temporary irradiance textures, by merging the two above loops:
	  <pre>
		for each wavelength lambda in the n wavelengths:
		  accumulate in the final illuminance texture (or, for the first
		  iteration, set this texture to) the product of the precomputed
		  irradiance at lambda (computed on the fly) with the value of the 3
		  sRGB color matching functions at lambda.
	  </pre>
	  <p>This is the method we use below, with 3 wavelengths per iteration instead
	  of 1, using <code>Precompute</code> to compute 3 irradiances values per
	  iteration, and <code>luminance_from_radiance</code> to multiply 3 irradiances
	  with the values of the 3 sRGB color matching functions at 3 different
	  wavelengths (yielding a 3x3 matrix).</li>
	</ul>

	<p>This yields the following implementation:
	*/

	void Model::Init(unsigned int num_scattering_orders) {
		// The precomputations require temporary textures, in particular to store the
		// contribution of one scattering order, which is needed to compute the next
		// order of scattering (the final precomputed textures store the sum of all
		// the scattering orders). We allocate them here, and destroy them at the end
		// of this method.
		/*
		uint32_t delta_irradiance_texture = NewTexture2d(
			IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
		uint32_t delta_rayleigh_scattering_texture = NewTexture3d(
			SCATTERING_TEXTURE_WIDTH,
			SCATTERING_TEXTURE_HEIGHT,
			SCATTERING_TEXTURE_DEPTH,
			rgb_format_supported_ ? GL_RGB : GL_RGBA,
			half_precision_);
		uint32_t delta_mie_scattering_texture = NewTexture3d(
			SCATTERING_TEXTURE_WIDTH,
			SCATTERING_TEXTURE_HEIGHT,
			SCATTERING_TEXTURE_DEPTH,
			rgb_format_supported_ ? GL_RGB : GL_RGBA,
			half_precision_);
		uint32_t delta_scattering_density_texture = NewTexture3d(
			SCATTERING_TEXTURE_WIDTH,
			SCATTERING_TEXTURE_HEIGHT,
			SCATTERING_TEXTURE_DEPTH,
			rgb_format_supported_ ? GL_RGB : GL_RGBA,
			half_precision_);
		// delta_multiple_scattering_texture is only needed to compute scattering
		// order 3 or more, while delta_rayleigh_scattering_texture and
		// delta_mie_scattering_texture are only needed to compute double scattering.
		// Therefore, to save memory, we can store delta_rayleigh_scattering_texture
		// and delta_multiple_scattering_texture in the same GPU texture.
		uint32_t delta_multiple_scattering_texture = delta_rayleigh_scattering_texture;

		// The precomputations also require a temporary framebuffer object, created
		// here (and destroyed at the end of this method).
		uint32_t fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// The actual precomputations depend on whether we want to store precomputed
		// irradiance or illuminance values.
		if (num_precomputed_wavelengths_ <= 3) {
			vec3 lambdas{ kLambdaR, kLambdaG, kLambdaB };
			mat3 luminance_from_radiance{ 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
			Precompute(fbo, delta_irradiance_texture, delta_rayleigh_scattering_texture,
				delta_mie_scattering_texture, delta_scattering_density_texture,
				delta_multiple_scattering_texture, lambdas, luminance_from_radiance,
				false, num_scattering_orders);
		}
		else {
			const double kLambdaMin = 360.0;
			const double kLambdaMax = 830.0;
			int num_iterations = (num_precomputed_wavelengths_ + 2) / 3;
			double dlambda = (kLambdaMax - kLambdaMin) / (3 * num_iterations);
			for (int i = 0; i < num_iterations; ++i) {
				vec3 lambdas{
				  kLambdaMin + (3 * i + 0.5) * dlambda,
				  kLambdaMin + (3 * i + 1.5) * dlambda,
				  kLambdaMin + (3 * i + 2.5) * dlambda
				};
				auto coeff = [dlambda](double lambda, int component) {
					// Note that we don't include MAX_LUMINOUS_EFFICACY here, to avoid
					// artefacts due to too large values when using half precision on GPU.
					// We add this term back in kAtmosphereShader, via
					// SKY_SPECTRAL_RADIANCE_TO_LUMINANCE (see also the comments in the
					// Model constructor).
					double x = CieColorMatchingFunctionTableValue(lambda, 1);
					double y = CieColorMatchingFunctionTableValue(lambda, 2);
					double z = CieColorMatchingFunctionTableValue(lambda, 3);
					return static_cast<float>((
						XYZ_TO_SRGB[component * 3] * x +
						XYZ_TO_SRGB[component * 3 + 1] * y +
						XYZ_TO_SRGB[component * 3 + 2] * z) * dlambda);
				};
				mat3 luminance_from_radiance{
				  coeff(lambdas[0], 0), coeff(lambdas[1], 0), coeff(lambdas[2], 0),
				  coeff(lambdas[0], 1), coeff(lambdas[1], 1), coeff(lambdas[2], 1),
				  coeff(lambdas[0], 2), coeff(lambdas[1], 2), coeff(lambdas[2], 2)
				};
				Precompute(fbo, delta_irradiance_texture,
					delta_rayleigh_scattering_texture, delta_mie_scattering_texture,
					delta_scattering_density_texture, delta_multiple_scattering_texture,
					lambdas, luminance_from_radiance, i > 0,
					num_scattering_orders);
			}

			// After the above iterations, the transmittance texture contains the
			// transmittance for the 3 wavelengths used at the last iteration. But we
			// want the transmittance at kLambdaR, kLambdaG, kLambdaB instead, so we
			// must recompute it here for these 3 wavelengths:
			std::string header = glsl_header_factory_({ kLambdaR, kLambdaG, kLambdaB });
			Program compute_transmittance(
				kVertexShader, header + kComputeTransmittanceShader);
			glFramebufferTexture(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, transmittance_texture_, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
			compute_transmittance.Use();
			DrawQuad({}, full_screen_quad_vao_);
		}

		// Delete the temporary resources allocated at the begining of this method.
		glUseProgram(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &delta_scattering_density_texture);
		glDeleteTextures(1, &delta_mie_scattering_texture);
		glDeleteTextures(1, &delta_rayleigh_scattering_texture);
		glDeleteTextures(1, &delta_irradiance_texture);
		assert(glGetError() == 0);
		*/
	}

	/*
	<p>The <code>SetProgramUniforms</code> method is straightforward: it simply
	binds the precomputed textures to the specified texture units, and then sets
	the corresponding uniforms in the user provided program to the index of these
	texture units.
	*/

	void Model::SetProgramUniforms(
		uint32_t program,
		uint32_t transmittance_texture_unit,
		uint32_t scattering_texture_unit,
		uint32_t irradiance_texture_unit,
		uint32_t single_mie_scattering_texture_unit) const {
		/*
		glActiveTexture(GL_TEXTURE0 + transmittance_texture_unit);
		glBindTexture(GL_TEXTURE_2D, transmittance_texture_);
		glUniform1i(glGetUniformLocation(program, "transmittance_texture"),
			transmittance_texture_unit);

		glActiveTexture(GL_TEXTURE0 + scattering_texture_unit);
		glBindTexture(GL_TEXTURE_3D, scattering_texture_);
		glUniform1i(glGetUniformLocation(program, "scattering_texture"),
			scattering_texture_unit);

		glActiveTexture(GL_TEXTURE0 + irradiance_texture_unit);
		glBindTexture(GL_TEXTURE_2D, irradiance_texture_);
		glUniform1i(glGetUniformLocation(program, "irradiance_texture"),
			irradiance_texture_unit);

		if (optional_single_mie_scattering_texture_ != 0) {
			glActiveTexture(GL_TEXTURE0 + single_mie_scattering_texture_unit);
			glBindTexture(GL_TEXTURE_3D, optional_single_mie_scattering_texture_);
			glUniform1i(glGetUniformLocation(program, "single_mie_scattering_texture"),
				single_mie_scattering_texture_unit);
		}
		*/
	}

	/*
	<p>The utility method <code>ConvertSpectrumToLinearSrgb</code> is implemented
	with a simple numerical integration of the given function, times the CIE color
	matching funtions (with an integration step of 1nm), followed by a matrix
	multiplication:
	*/

	void Model::ConvertSpectrumToLinearSrgb(
		const std::vector<double>& wavelengths,
		const std::vector<double>& spectrum,
		double* r, double* g, double* b) {
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		const int dlambda = 1;
		for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
			double value = Interpolate(wavelengths, spectrum, lambda);
			x += CieColorMatchingFunctionTableValue(lambda, 1) * value;
			y += CieColorMatchingFunctionTableValue(lambda, 2) * value;
			z += CieColorMatchingFunctionTableValue(lambda, 3) * value;
		}
		*r = MAX_LUMINOUS_EFFICACY *
			(XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
		*g = MAX_LUMINOUS_EFFICACY *
			(XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
		*b = MAX_LUMINOUS_EFFICACY *
			(XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
	}

	/*
	<p>Finally, we provide the actual implementation of the precomputation algorithm
	described in Algorithm 4.1 of
	<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. Each step is
	explained by the inline comments below.
	*/
	void Model::Precompute(
		const TShared<TextureResource>& delta_irradiance_texture,
		const TShared<TextureResource>& delta_rayleigh_scattering_texture,
		const TShared<TextureResource>& delta_mie_scattering_texture,
		const TShared<TextureResource>& delta_scattering_density_texture,
		const TShared<TextureResource>& delta_multiple_scattering_texture,
		const Float3& lambdas,
		const MatrixFloat3x3& luminance_from_radiance,
		bool blend,
		unsigned int num_scattering_orders) {
		/*
		// The precomputations require specific GLSL programs, for each precomputation
		// step. We create and compile them here (they are automatically destroyed
		// when this method returns, via the Program destructor).
		std::string header = glsl_header_factory_(lambdas);
		Program compute_transmittance(
			kVertexShader, header + kComputeTransmittanceShader);
		Program compute_direct_irradiance(
			kVertexShader, header + kComputeDirectIrradianceShader);
		Program compute_single_scattering(kVertexShader, kGeometryShader,
			header + kComputeSingleScatteringShader);
		Program compute_scattering_density(kVertexShader, kGeometryShader,
			header + kComputeScatteringDensityShader);
		Program compute_indirect_irradiance(
			kVertexShader, header + kComputeIndirectIrradianceShader);
		Program compute_multiple_scattering(kVertexShader, kGeometryShader,
			header + kComputeMultipleScatteringShader);

		const uint32_t kDrawBuffers[4] = {
		  GL_COLOR_ATTACHMENT0,
		  GL_COLOR_ATTACHMENT1,
		  GL_COLOR_ATTACHMENT2,
		  GL_COLOR_ATTACHMENT3
		};
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

		// Compute the transmittance, and store it in transmittance_texture_.
		glFramebufferTexture(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, transmittance_texture_, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
		compute_transmittance.Use();
		DrawQuad({}, full_screen_quad_vao_);

		// Compute the direct irradiance, store it in delta_irradiance_texture and,
		// depending on 'blend', either initialize irradiance_texture_ with zeros or
		// leave it unchanged (we don't want the direct irradiance in
		// irradiance_texture_, but only the irradiance from the sky).
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			delta_irradiance_texture, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			irradiance_texture_, 0);
		glDrawBuffers(2, kDrawBuffers);
		glViewport(0, 0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
		compute_direct_irradiance.Use();
		compute_direct_irradiance.BindTexture2d(
			"transmittance_texture", transmittance_texture_, 0);
		DrawQuad({ false, blend }, full_screen_quad_vao_);

		// Compute the rayleigh and mie single scattering, store them in
		// delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
		// either store them or accumulate them in scattering_texture_ and
		// optional_single_mie_scattering_texture_.
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			delta_rayleigh_scattering_texture, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			delta_mie_scattering_texture, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
			scattering_texture_, 0);
		if (optional_single_mie_scattering_texture_ != 0) {
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
				optional_single_mie_scattering_texture_, 0);
			glDrawBuffers(4, kDrawBuffers);
		}
		else {
			glDrawBuffers(3, kDrawBuffers);
		}
		glViewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
		compute_single_scattering.Use();
		compute_single_scattering.BindMat3(
			"luminance_from_radiance", luminance_from_radiance);
		compute_single_scattering.BindTexture2d(
			"transmittance_texture", transmittance_texture_, 0);
		for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
			compute_single_scattering.BindInt("layer", layer);
			DrawQuad({ false, false, blend, blend }, full_screen_quad_vao_);
		}

		// Compute the 2nd, 3rd and 4th order of scattering, in sequence.
		for (unsigned int scattering_order = 2;
			scattering_order <= num_scattering_orders;
			++scattering_order) {
			// Compute the scattering density, and store it in
			// delta_scattering_density_texture.
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				delta_scattering_density_texture, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 0, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, 0, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, 0, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
			compute_scattering_density.Use();
			compute_scattering_density.BindTexture2d(
				"transmittance_texture", transmittance_texture_, 0);
			compute_scattering_density.BindTexture3d(
				"single_rayleigh_scattering_texture",
				delta_rayleigh_scattering_texture,
				1);
			compute_scattering_density.BindTexture3d(
				"single_mie_scattering_texture", delta_mie_scattering_texture, 2);
			compute_scattering_density.BindTexture3d(
				"multiple_scattering_texture", delta_multiple_scattering_texture, 3);
			compute_scattering_density.BindTexture2d(
				"irradiance_texture", delta_irradiance_texture, 4);
			compute_scattering_density.BindInt("scattering_order", scattering_order);
			for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
				compute_scattering_density.BindInt("layer", layer);
				DrawQuad({}, full_screen_quad_vao_);
			}

			// Compute the indirect irradiance, store it in delta_irradiance_texture and
			// accumulate it in irradiance_texture_.
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				delta_irradiance_texture, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
				irradiance_texture_, 0);
			glDrawBuffers(2, kDrawBuffers);
			glViewport(0, 0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
			compute_indirect_irradiance.Use();
			compute_indirect_irradiance.BindMat3(
				"luminance_from_radiance", luminance_from_radiance);
			compute_indirect_irradiance.BindTexture3d(
				"single_rayleigh_scattering_texture",
				delta_rayleigh_scattering_texture,
				0);
			compute_indirect_irradiance.BindTexture3d(
				"single_mie_scattering_texture", delta_mie_scattering_texture, 1);
			compute_indirect_irradiance.BindTexture3d(
				"multiple_scattering_texture", delta_multiple_scattering_texture, 2);
			compute_indirect_irradiance.BindInt("scattering_order",
				scattering_order - 1);
			DrawQuad({ false, true }, full_screen_quad_vao_);

			// Compute the multiple scattering, store it in
			// delta_multiple_scattering_texture, and accumulate it in
			// scattering_texture_.
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				delta_multiple_scattering_texture, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
				scattering_texture_, 0);
			glDrawBuffers(2, kDrawBuffers);
			glViewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
			compute_multiple_scattering.Use();
			compute_multiple_scattering.BindMat3(
				"luminance_from_radiance", luminance_from_radiance);
			compute_multiple_scattering.BindTexture2d(
				"transmittance_texture", transmittance_texture_, 0);
			compute_multiple_scattering.BindTexture3d(
				"scattering_density_texture", delta_scattering_density_texture, 1);
			for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
				compute_multiple_scattering.BindInt("layer", layer);
				DrawQuad({ false, true }, full_screen_quad_vao_);
			}
		}
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 0, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, 0, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, 0, 0);
		*/
	}

}  // namespace atmosphere
