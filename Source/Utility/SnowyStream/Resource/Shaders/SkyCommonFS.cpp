#define USE_SWIZZLE
#include "SkyCommonFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

String SkyCommonFS::GetPredefines() {
	return "\n\
#define Length float \n\
#define Wavelength float \n\
#define Angle float \n\
#define SolidAngle float \n\
#define Power float \n\
#define LuminousPower float \n\
#define Number float \n\
#define InverseLength float \n\
#define Area float \n\
#define Volume float \n\
#define NumberDensity float \n\
#define Irradiance float \n\
#define Radiance float \n\
#define SpectralPower float \n\
#define SpectralIrradiance float \n\
#define SpectralRadiance float \n\
#define SpectralRadianceDensity float \n\
#define ScatteringCoefficient float \n\
#define InverseSolidAngle float \n\
#define LuminousIntensity float \n\
#define Luminance float \n\
#define Illuminance float \n\
#define AbstractSpectrum float3 \n\
#define DimensionlessSpectrum float3 \n\
#define PowerSpectrum float3 \n\
#define IrradianceSpectrum float3 \n\
#define RadianceSpectrum float3 \n\
#define RadianceDensitySpectrum float3 \n\
#define ScatteringSpectrum float3 \n\n\
#define Position float3 \n\
#define Direction float3 \n\
#define Luminance3 float3 \n\
#define Illuminance3 float3 \n\
const Length m = 1.0; \n\
const Wavelength nm = 1.0; \n\
const Angle rad = 1.0; \n\
const SolidAngle sr = 1.0; \n\
const Power watt = 1.0; \n\
const LuminousPower lm = 1.0; \n\
const Length km = 1000.0 * m; \n\
const Area m2 = m * m; \n\
const Volume m3 = m * m * m; \n\
const Angle pi = PI * rad; \n\
const Angle deg = pi / 180.0; \n\
const Irradiance watt_per_square_meter = watt / m2; \n\
const Radiance watt_per_square_meter_per_sr = watt / (m2 * sr); \n\
const SpectralIrradiance watt_per_square_meter_per_nm = watt / (m2 * nm); \n\
const SpectralRadiance watt_per_square_meter_per_sr_per_nm = watt / (m2 * sr * nm); \n\
const SpectralRadianceDensity watt_per_cubic_meter_per_sr_per_nm = watt / (m3 * sr * nm); \n\
const LuminousIntensity cd = lm / sr; \n\
const LuminousIntensity kcd = 1000.0 * cd; \n\
const Luminance cd_per_square_meter = cd / m2; \n\
const Luminance kcd_per_square_meter = kcd / m2; \n\
#define assert(f) \n\
#define ClampCosine(f) clamp(f, Number(-1.0), Number(1.0)) \n\
#define ClampDistance(f) max(f, 0.0 * m) \n\
#define ClampRadius(atmosphere, r) clamp(r, atmosphere.bottom_radius, atmosphere.top_radius) \n\
#define SafeSqrt(r) sqrt(max(r, 0.0 * m2)) \n\
";
}

#define ClampCosine(f) clamp(f, Number(-1.0), Number(1.0))
#define ClampDistance(f) max(f, Number(0.0))
#define ClampRadius(atmosphere, r) clamp(r, atmosphere.bottom_radius, atmosphere.top_radius)
#define SafeSqrt(r) sqrt(max(r, Number(0.0)))

const Length m = 1.0;
const Wavelength nm = 1.0;
const Angle rad = 1.0;
const SolidAngle sr = 1.0;
const Power watt = 1.0;
const LuminousPower lm = 1.0;
const Length km = 1000.0 * m;
const Area m2 = m * m;
const Volume m3 = m * m * m;
const Angle pi = PI * rad;
const Angle deg = pi / 180.0;
const Irradiance watt_per_square_meter = watt / m2;
const Radiance watt_per_square_meter_per_sr = watt / (m2 * sr);
const SpectralIrradiance watt_per_square_meter_per_nm = watt / (m2 * nm);
const SpectralRadiance watt_per_square_meter_per_sr_per_nm =
watt / (m2 * sr * nm);
const SpectralRadianceDensity watt_per_cubic_meter_per_sr_per_nm =
watt / (m3 * sr * nm);
const LuminousIntensity cd = lm / sr;
const LuminousIntensity kcd = 1000.0 * cd;
const Luminance cd_per_square_meter = cd / m2;
const Luminance kcd_per_square_meter = kcd / m2;

String SkyCommonFS::DistanceToTopAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu) {
	return UnifyShaderCode(
		assert(r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	Area discriminant = r * r * (mu * mu - 1.0) +
		atmosphere.top_radius * atmosphere.top_radius;
	retValue = ClampDistance(-r * mu + SafeSqrt(discriminant));
	);
}

String SkyCommonFS::DistanceToBottomAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	Area discriminant = r * r * (mu * mu - 1.0) +
		atmosphere.bottom_radius * atmosphere.bottom_radius;
	retValue = ClampDistance(-r * mu - SafeSqrt(discriminant));
	);
}

String SkyCommonFS::RayIntersectsGround(bool& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	retValue = mu < 0.0 && r * r * (mu * mu - 1.0) +
		atmosphere.bottom_radius * atmosphere.bottom_radius >= 0.0 * m2;
	);
}

String SkyCommonFS::GetLayerDensity(Number& retValue, const DensityProfileLayer& layer, Length altitude) {
	return UnifyShaderCode(
		Number density = layer.exp_term * exp(layer.exp_scale * altitude) +
		layer.linear_term * altitude + layer.constant_term;
	retValue = clamp(density, Number(0.0), Number(1.0));
	);
}

String SkyCommonFS::GetProfileDensity(Number& retValue, const DensityProfile& profile, Length altitude) {
	return UnifyShaderCode(
		Number a;
		Number b;
	GetLayerDensity(a, profile.layer_bottom, altitude);
	GetLayerDensity(b, profile.layer_top, altitude);
	retValue = altitude < profile.layer_bottom.width ? a : b;
	);
}

String SkyCommonFS::ComputeOpticalLengthToTopAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, const DensityProfile& profile, Length r, Number mu) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	// Number of intervals for the numerical integration.
	const int SAMPLE_COUNT = 500;
	// The integration step, i.e. the length of each integration interval.
	Length dx;
	DistanceToTopAtmosphereBoundary(dx, atmosphere, r, mu);
	dx /= Number(SAMPLE_COUNT);
	// Integration loop.
	Length result = 0.0 * m;
	for (int i = 0; i <= SAMPLE_COUNT; ++i) {
		Length d_i = Number(i) * dx;
		// Distance between the current sample point and the planet center.
		Length r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
		// Number density at the current sample point (divided by the number density
		// at the bottom of the atmosphere, yielding a dimensionless number).
		Number y_i;
		GetProfileDensity(y_i, profile, r_i - atmosphere.bottom_radius);
		// Sample weight (from the trapezoidal rule).
		Number weight_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0;
		result += y_i * weight_i * dx;
	}

	retValue = result;
	);
}

String SkyCommonFS::ComputeTransmittanceToTopAtmosphereBoundary(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	Length a;
	Length b;
	Length c;
	ComputeOpticalLengthToTopAtmosphereBoundary(a, atmosphere, atmosphere.rayleigh_density, r, mu);
	ComputeOpticalLengthToTopAtmosphereBoundary(b, atmosphere, atmosphere.mie_density, r, mu);
	ComputeOpticalLengthToTopAtmosphereBoundary(c, atmosphere, atmosphere.absorption_density, r, mu);

	retValue = exp(-(atmosphere.rayleigh_scattering * a + atmosphere.mie_extinction * b + atmosphere.absorption_extinction * c));
	);
}

String SkyCommonFS::GetTextureCoordFromUnitRange(Number& retValue, Number u, float texture_size) {
	return UnifyShaderCode(retValue = 0.5 / Number(texture_size) + u * (1.0 - 1.0 / Number(texture_size)););
}

String SkyCommonFS::GetUnitRangeFromTextureCoord(Number& retValue, Number u, float texture_size) {
	return UnifyShaderCode(retValue = (u - 0.5 / Number(texture_size)) / (1.0 - 1.0 / Number(texture_size)););
}

String SkyCommonFS::GetTransmittanceTextureUvFromRMu(Float2& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon.
	Length rho =
		SafeSqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
	// and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
	Length d;
	DistanceToTopAtmosphereBoundary(d, atmosphere, r, mu);
	Length d_min = atmosphere.top_radius - r;
	Length d_max = rho + H;
	Number x_mu = (d - d_min) / (d_max - d_min);
	Number x_r = rho / H;
	Number vx;
	Number vy;
	GetTextureCoordFromUnitRange(vx, x_mu, TRANSMITTANCE_TEXTURE_WIDTH);
	GetTextureCoordFromUnitRange(vy, x_r, TRANSMITTANCE_TEXTURE_HEIGHT);

	retValue = float2(vx, vy);
	);
}

String SkyCommonFS::GetRMuFromTransmittanceTextureUv(const AtmosphereParameters& atmosphere, const Float2& uv, Length& r, Number& mu) {
	return UnifyShaderCode(
		assert(uv.x >= 0.0 && uv.x <= 1.0);
	assert(uv.y >= 0.0 && uv.y <= 1.0);
	Number x_mu;
	GetUnitRangeFromTextureCoord(x_mu, uv.x, TRANSMITTANCE_TEXTURE_WIDTH);
	Number x_r;
	GetUnitRangeFromTextureCoord(x_r, uv.y, TRANSMITTANCE_TEXTURE_HEIGHT);
	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon, from which we can compute r:
	Length rho = H * x_r;
	r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
	// and maximum values over all mu - obtained for (r,1) and (r,mu_horizon) -
	// from which we can recover mu:
	Length d_min = atmosphere.top_radius - r;
	Length d_max = rho + H;
	Length d = d_min + x_mu * (d_max - d_min);
	mu = d == 0.0 * m ? Number(1.0) : (H * H - rho * rho - d * d) / (2.0 * r * d);
	mu = ClampCosine(mu);
	);
}

String SkyCommonFS::ComputeTransmittanceToTopAtmosphereBoundaryTexture(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const Float2& frag_coord) {
	return UnifyShaderCode(
		const float2 TRANSMITTANCE_TEXTURE_SIZE =
		float2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
	Length r;
	Number mu;
	GetRMuFromTransmittanceTextureUv(
		atmosphere, frag_coord / TRANSMITTANCE_TEXTURE_SIZE, r, mu);
	ComputeTransmittanceToTopAtmosphereBoundary(retValue, atmosphere, r, mu);
	);
}

String SkyCommonFS::GetTransmittanceToTopAtmosphereBoundary(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const TransmittanceTexture& transmittance_texture, Length r, Number mu) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	float2 uv;
	GetTransmittanceTextureUvFromRMu(uv, atmosphere, r, mu);
	retValue = DimensionlessSpectrum(texture(transmittance_texture, uv));
	);
}

String SkyCommonFS::GetTransmittance(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const TransmittanceTexture& transmittance_texture, Length r, Number mu, Length d, bool ray_r_mu_intersects_ground) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(d >= 0.0 * m);

	Length r_d = ClampRadius(atmosphere, Length(sqrt(d * d + 2.0 * r * mu * d + r * r)));
	Number mu_d = ClampCosine((r * mu + d) / r_d);

	if (ray_r_mu_intersects_ground) {
		DimensionlessSpectrum a;
		DimensionlessSpectrum b;
		GetTransmittanceToTopAtmosphereBoundary(a, atmosphere, transmittance_texture, r_d, -mu_d);
		GetTransmittanceToTopAtmosphereBoundary(b, atmosphere, transmittance_texture, r, -mu);
		retValue = min(a / b, DimensionlessSpectrum(1.0));
	} else {
		DimensionlessSpectrum a;
		DimensionlessSpectrum b;
		GetTransmittanceToTopAtmosphereBoundary(a, atmosphere, transmittance_texture, r, mu);
		GetTransmittanceToTopAtmosphereBoundary(b, atmosphere, transmittance_texture, r_d, mu_d);
		retValue = min(a / b, DimensionlessSpectrum(1.0));
	}
	);
}

String SkyCommonFS::GetTransmittanceToSun(DimensionlessSpectrum& retValue, const AtmosphereParameters& atmosphere, const TransmittanceTexture& transmittance_texture, Length r, Number mu_s) {
	return UnifyShaderCode(
		Number sin_theta_h = atmosphere.bottom_radius / r;
	Number cos_theta_h = -Number(sqrt(max(Number(1.0) - sin_theta_h * sin_theta_h, Number(0.0))));
	GetTransmittanceToTopAtmosphereBoundary(retValue,
		atmosphere, transmittance_texture, r, mu_s);
	retValue = retValue * smoothstep(-sin_theta_h * atmosphere.sun_angular_radius / rad,
		sin_theta_h * atmosphere.sun_angular_radius / rad,
		mu_s - cos_theta_h);
	);
}

String SkyCommonFS::ComputeSingleScatteringIntegrand(
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	Length r, Number mu, Number mu_s, Number nu, Length d,
	bool ray_r_mu_intersects_ground,
	DimensionlessSpectrum& rayleigh, DimensionlessSpectrum& mie) {

	return UnifyShaderCode(
		Length r_d = ClampRadius(atmosphere, sqrt(d * d + Number(2.0) * r * mu * d + r * r));
	Number mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);
	DimensionlessSpectrum transmittance;
	GetTransmittance(transmittance,
		atmosphere, transmittance_texture, r, mu, d,
		ray_r_mu_intersects_ground);
	DimensionlessSpectrum a;
		GetTransmittanceToSun(a,
			atmosphere, transmittance_texture, r_d, mu_s_d);
	transmittance = transmittance * a;
	Number v;
	GetProfileDensity(v, atmosphere.rayleigh_density, r_d - atmosphere.bottom_radius);
	rayleigh = transmittance * v;
	GetProfileDensity(v, atmosphere.mie_density, r_d - atmosphere.bottom_radius);
	mie = transmittance * v;
	);
}

String SkyCommonFS::DistanceToNearestAtmosphereBoundary(Length& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu, bool ray_r_mu_intersects_ground) {
	return UnifyShaderCode(
		if (ray_r_mu_intersects_ground) {
			DistanceToBottomAtmosphereBoundary(retValue, atmosphere, r, mu);
		} else {
			DistanceToTopAtmosphereBoundary(retValue, atmosphere, r, mu);
		}
	);
}

String SkyCommonFS::ComputeSingleScattering(
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground,
	IrradianceSpectrum& rayleigh, IrradianceSpectrum& mie) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);

	// Number of intervals for the numerical integration.
	const int SAMPLE_COUNT = 50;
	// The integration step, i.e. the length of each integration interval.
	Length dx;
	DistanceToNearestAtmosphereBoundary(dx, atmosphere, r, mu,
		ray_r_mu_intersects_ground);
	dx = dx / Number(SAMPLE_COUNT);
	// Integration loop.
	DimensionlessSpectrum rayleigh_sum = DimensionlessSpectrum(0.0);
	DimensionlessSpectrum mie_sum = DimensionlessSpectrum(0.0);
	for (int i = 0; i <= SAMPLE_COUNT; ++i) {
		Length d_i = Number(i) * dx;
		// The Rayleigh and Mie single scattering at the current sample point.
		DimensionlessSpectrum rayleigh_i;
		DimensionlessSpectrum mie_i;
		ComputeSingleScatteringIntegrand(atmosphere, transmittance_texture,
			r, mu, mu_s, nu, d_i, ray_r_mu_intersects_ground, rayleigh_i, mie_i);
		// Sample weight (from the trapezoidal rule).
		Number weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
		rayleigh_sum += rayleigh_i * weight_i;
		mie_sum += mie_i * weight_i;
	}
	rayleigh = rayleigh_sum * dx * atmosphere.solar_irradiance *
		atmosphere.rayleigh_scattering;
	mie = mie_sum * dx * atmosphere.solar_irradiance * atmosphere.mie_scattering;
	);
}

String SkyCommonFS::RayleighPhaseFunction(InverseSolidAngle& retValue, Number nu) {
	return UnifyShaderCode(
		InverseSolidAngle k = 3.0 / (16.0 * PI * sr);
	retValue = k * (1.0 + nu * nu);
	);
}

String SkyCommonFS::MiePhaseFunction(InverseSolidAngle& retValue, Number g, Number nu) {
	return UnifyShaderCode(
		InverseSolidAngle k = 3.0 / (8.0 * PI * sr) * (1.0 - g * g) / (2.0 + g * g);
	retValue = k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
	);
}

String SkyCommonFS::GetScatteringTextureUvwzFromRMuMuSNu(Float4& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu, Number mu_s, Number nu, bool ray_r_mu_intersects_ground) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);

	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon.
	Length rho =
		SafeSqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);
	Number u_r;
	GetTextureCoordFromUnitRange(u_r, rho / H, SCATTERING_TEXTURE_R_SIZE);

	// Discriminant of the quadratic equation for the intersections of the ray
	// (r,mu) with the ground (see RayIntersectsGround).
	Length r_mu = r * mu;
	Area discriminant =
		r_mu * r_mu - r * r + atmosphere.bottom_radius * atmosphere.bottom_radius;
	Number u_mu;
	if (ray_r_mu_intersects_ground) {
		// Distance to the ground for the ray (r,mu), and its minimum and maximum
		// values over all mu - obtained for (r,-1) and (r,mu_horizon).
		Length d = -r_mu - SafeSqrt(discriminant);
		Length d_min = r - atmosphere.bottom_radius;
		Length d_max = rho;
		Length uv;
		GetTextureCoordFromUnitRange(uv, d_max == d_min ? 0.0 :
			(d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
		u_mu = 0.5 - 0.5 * uv;
	} else {
		// Distance to the top atmosphere boundary for the ray (r,mu), and its
		// minimum and maximum values over all mu - obtained for (r,1) and
		// (r,mu_horizon).
		Length d = -r_mu + SafeSqrt(discriminant + H * H);
		Length d_min = atmosphere.top_radius - r;
		Length d_max = rho + H;
		Length uv;
		GetTextureCoordFromUnitRange(uv, (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
		u_mu = 0.5 + 0.5 * uv;
	}

	Length d;
	DistanceToTopAtmosphereBoundary(d,
		atmosphere, atmosphere.bottom_radius, mu_s);
	Length d_min = atmosphere.top_radius - atmosphere.bottom_radius;
	Length d_max = H;
	Number a = (d - d_min) / (d_max - d_min);
	Length D;
	DistanceToTopAtmosphereBoundary(D,
		atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
	Number A = (D - d_min) / (d_max - d_min);
	// An ad-hoc function equal to 0 for mu_s = mu_s_min (because then d = D and
	// thus a = A), equal to 1 for mu_s = 1 (because then d = d_min and thus
	// a = 0), and with a large slope around mu_s = 0, to get more texture 
	// samples near the horizon.
	Number u_mu_s;
	GetTextureCoordFromUnitRange(u_mu_s, max(1.0 - a / A, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);

	Number u_nu = (nu + 1.0) / 2.0;
	retValue = float4(u_nu, u_mu_s, u_mu, u_r);
	);
}

String SkyCommonFS::GetRMuMuSNuFromScatteringTextureUvwz(const AtmosphereParameters& atmosphere,
	const Float4& uvwz, Length& r, Number& mu, Number& mu_s,
	Number& nu, bool& ray_r_mu_intersects_ground) {

	return UnifyShaderCode(
		assert(uvwz.x >= 0.0 && uvwz.x <= 1.0);
	assert(uvwz.y >= 0.0 && uvwz.y <= 1.0);
	assert(uvwz.z >= 0.0 && uvwz.z <= 1.0);
	assert(uvwz.w >= 0.0 && uvwz.w <= 1.0);

	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon.
	Length rho;
	GetUnitRangeFromTextureCoord(rho, uvwz.w, SCATTERING_TEXTURE_R_SIZE);
	rho = rho * H;
	r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);

	if (uvwz.z < 0.5) {
		// Distance to the ground for the ray (r,mu), and its minimum and maximum
		// values over all mu - obtained for (r,-1) and (r,mu_horizon) - from which
		// we can recover mu:
		Length d_min = r - atmosphere.bottom_radius;
		Length d_max = rho;
		Length v;
		GetUnitRangeFromTextureCoord(v, Number(1.0) - Number(2.0) * uvwz.z, SCATTERING_TEXTURE_MU_SIZE / 2);
		Length d = d_min + (d_max - d_min) * v;
		mu = d == 0.0 * m ? Number(-1.0) :
			ClampCosine(-(rho * rho + d * d) / (Number(2.0) * r * d));
		ray_r_mu_intersects_ground = true;
	} else {
		// Distance to the top atmosphere boundary for the ray (r,mu), and its
		// minimum and maximum values over all mu - obtained for (r,1) and
		// (r,mu_horizon) - from which we can recover mu:
		Length d_min = atmosphere.top_radius - r;
		Length d_max = rho + H;
		Length v;
		GetUnitRangeFromTextureCoord(v, Number(2.0) * uvwz.z - Number(1.0), SCATTERING_TEXTURE_MU_SIZE / 2);
		Length d = d_min + (d_max - d_min) * v;
		mu = d == 0.0 * m ? Number(1.0) :
			ClampCosine((H * H - rho * rho - d * d) / (Number(2.0) * r * d));
		ray_r_mu_intersects_ground = false;
	}

	Number x_mu_s;
	GetUnitRangeFromTextureCoord(x_mu_s, uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
	Length d_min = atmosphere.top_radius - atmosphere.bottom_radius;
	Length d_max = H;
	Length D;
	DistanceToTopAtmosphereBoundary(D, atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
	Number A = (D - d_min) / (d_max - d_min);
	Number a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
	Length d = d_min + min(a, A) * (d_max - d_min);
	mu_s = d == 0.0 * m ? Number(1.0) :
		ClampCosine((H * H - d * d) / (Number(2.0) * atmosphere.bottom_radius * d));

	nu = ClampCosine(uvwz.x * Number(2.0) - Number(1.0));
	);
}

String SkyCommonFS::GetRMuMuSNuFromScatteringTextureFragCoord(
	const AtmosphereParameters& atmosphere, const Float3& frag_coord,
	Length& r, Number& mu, Number& mu_s, Number& nu,
	bool& ray_r_mu_intersects_ground) {
	return UnifyShaderCode(
		const float4 SCATTERING_TEXTURE_SIZE = float4(
			SCATTERING_TEXTURE_NU_SIZE - 1,
			SCATTERING_TEXTURE_MU_S_SIZE,
			SCATTERING_TEXTURE_MU_SIZE,
			SCATTERING_TEXTURE_R_SIZE);
	Number frag_coord_nu =
		floor(frag_coord.x / Number(SCATTERING_TEXTURE_MU_S_SIZE));
	Number frag_coord_mu_s =
		mod(frag_coord.x, Number(SCATTERING_TEXTURE_MU_S_SIZE));
	float4 uvwz =
		float4(frag_coord_nu, frag_coord_mu_s, frag_coord.y, frag_coord.z) /
		SCATTERING_TEXTURE_SIZE;
	GetRMuMuSNuFromScatteringTextureUvwz(
		atmosphere, uvwz, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	// Clamp nu to its valid range of values, given mu and mu_s.
	nu = clamp(nu, mu * mu_s - Number(sqrt((Number(1.0) - mu * mu) * (Number(1.0) - mu_s * mu_s))),
		mu * mu_s + Number(sqrt((Number(1.0) - mu * mu) * (Number(1.0) - mu_s * mu_s))));
	);
}

String SkyCommonFS::ComputeSingleScatteringTexture(const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture, const Float3& frag_coord,
	IrradianceSpectrum& rayleigh, IrradianceSpectrum& mie) {
	return UnifyShaderCode(
		Length r;
	Number mu;
	Number mu_s;
	Number nu;
	bool ray_r_mu_intersects_ground;
	GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	ComputeSingleScattering(atmosphere, transmittance_texture,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground, rayleigh, mie);
	);
}

String SkyCommonFS::GetScatteringAbstract(AbstractSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const AbstractSpectrumTexture& scattering_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground) {
	return UnifyShaderCode(
		float4 uvwz;
	GetScatteringTextureUvwzFromRMuMuSNu(uvwz, atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	Number tex_coord_x = uvwz.x * Number(SCATTERING_TEXTURE_NU_SIZE - 1);
	Number tex_x = floor(tex_coord_x);
	Number lp = tex_coord_x - tex_x;
	float3 uvw0 = float3((tex_x + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	retValue = AbstractSpectrum(texture(scattering_texture, uvw0).xyz * (float(1.0) - lp) +
		texture(scattering_texture, uvw1).xyz * lp);
	);
}

String SkyCommonFS::GetScatteringReduced(RadianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const ReducedScatteringTexture& single_rayleigh_scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	const ScatteringTexture& multiple_scattering_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground,
	int scattering_order) {
	return UnifyShaderCode(
		if (scattering_order == 1) {
			IrradianceSpectrum rayleigh;
			GetScatteringAbstract(rayleigh,
				atmosphere, single_rayleigh_scattering_texture, r, mu, mu_s, nu,
				ray_r_mu_intersects_ground);
			IrradianceSpectrum mie;
			GetScatteringAbstract(mie,
				atmosphere, single_mie_scattering_texture, r, mu, mu_s, nu,
				ray_r_mu_intersects_ground);
			InverseSolidAngle a;
			RayleighPhaseFunction(a, nu);
			InverseSolidAngle b;
			MiePhaseFunction(b, atmosphere.mie_phase_function_g, nu);
			retValue = rayleigh * a + mie * b;
		} else {
			GetScatteringAbstract(retValue,
				atmosphere, multiple_scattering_texture, r, mu, mu_s, nu,
				ray_r_mu_intersects_ground);
		}
	);
}

String SkyCommonFS::ComputeScatteringDensity(RadianceDensitySpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const ReducedScatteringTexture& single_rayleigh_scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	const ScatteringTexture& multiple_scattering_texture,
	const IrradianceTexture& irradiance_texture,
	Length r, Number mu, Number mu_s, Number nu, int scattering_order) {

	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);
	assert(scattering_order >= 2);

	// Compute unit direction vectors for the zenith, the view direction omega and
	// and the sun direction omega_s, such that the cosine of the view-zenith
	// angle is mu, the cosine of the sun-zenith angle is mu_s, and the cosine of
	// the view-sun angle is nu. The goal is to simplify computations below.
	float3 zenith_direction = float3(0.0, 0.0, 1.0);
	float3 omega = float3(sqrt(1.0 - mu * mu), 0.0, mu);
	Number sun_dir_x = omega.x == 0.0 ? 0.0 : (nu - mu * mu_s) / omega.x;
	Number sun_dir_y = sqrt(max(1.0 - sun_dir_x * sun_dir_x - mu_s * mu_s, 0.0));
	float3 omega_s = float3(sun_dir_x, sun_dir_y, mu_s);

	const int SAMPLE_COUNT = 16;
	const Angle dphi = pi / Number(SAMPLE_COUNT);
	const Angle dtheta = pi / Number(SAMPLE_COUNT);
	RadianceDensitySpectrum rayleigh_mie =
		RadianceDensitySpectrum(0.0 * watt_per_cubic_meter_per_sr_per_nm);

	// Nested loops for the integral over all the incident directions omega_i.
	for (int l = 0; l < SAMPLE_COUNT; ++l) {
		Angle theta = (Number(l) + 0.5) * dtheta;
		Number cos_theta = cos(theta);
		Number sin_theta = sin(theta);
		bool ray_r_theta_intersects_ground;
		RayIntersectsGround(ray_r_theta_intersects_ground, atmosphere, r, cos_theta);

		// The distance and transmittance to the ground only depend on theta, so we
		// can compute them in the outer loop for efficiency.
		Length distance_to_ground = 0.0 * m;
		DimensionlessSpectrum transmittance_to_ground = DimensionlessSpectrum(0.0);
		DimensionlessSpectrum ground_albedo = DimensionlessSpectrum(0.0);
		if (ray_r_theta_intersects_ground) {
			DistanceToBottomAtmosphereBoundary(distance_to_ground, atmosphere, r, cos_theta);
			GetTransmittance(transmittance_to_ground, atmosphere, transmittance_texture, r, cos_theta,
				distance_to_ground, true /* ray_intersects_ground */);
			ground_albedo = atmosphere.ground_albedo;
		}

		for (int m = 0; m < 2 * SAMPLE_COUNT; ++m) {
			Angle phi = (Number(m) + 0.5) * dphi;
			float3 omega_i =
				float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
			SolidAngle domega_i = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;

			// The radiance L_i arriving from direction omega_i after n-1 bounces is
			// the sum of a term given by the precomputed scattering texture for the
			// (n-1)-th order:
			Number nu1 = dot(omega_s, omega_i);
			RadianceSpectrum incident_radiance;
			GetScatteringReduced(incident_radiance, atmosphere,
				single_rayleigh_scattering_texture, single_mie_scattering_texture,
				multiple_scattering_texture, r, omega_i.z, mu_s, nu1,
				ray_r_theta_intersects_ground, scattering_order - 1);

			// and of the contribution from the light paths with n-1 bounces and whose
			// last bounce is on the ground. This contribution is the product of the
			// transmittance to the ground, the ground albedo, the ground BRDF, and
			// the irradiance received on the ground after n-2 bounces.
			float3 ground_normal =
				normalize(zenith_direction * r + omega_i * distance_to_ground);
			IrradianceSpectrum ground_irradiance;
			GetIrradiance(ground_irradiance,
				atmosphere, irradiance_texture, atmosphere.bottom_radius,
				dot(ground_normal, omega_s));
			incident_radiance += transmittance_to_ground *
				ground_albedo * (Number(1.0) / (PI * sr)) * ground_irradiance;

			// The radiance finally scattered from direction omega_i towards direction
			// -omega is the product of the incident radiance, the scattering
			// coefficient, and the phase function for directions omega and omega_i
			// (all this summed over all particle types, i.e. Rayleigh and Mie).
			Number nu2 = dot(omega, omega_i);
			Number rayleigh_density;
			GetProfileDensity(rayleigh_density,
				atmosphere.rayleigh_density, r - atmosphere.bottom_radius);
			Number mie_density;
			GetProfileDensity(mie_density,
				atmosphere.mie_density, r - atmosphere.bottom_radius);
			InverseSolidAngle a;
			InverseSolidAngle b;
			RayleighPhaseFunction(a, nu2);
			MiePhaseFunction(b, atmosphere.mie_phase_function_g, nu2);
			rayleigh_mie += incident_radiance * (
				atmosphere.rayleigh_scattering * rayleigh_density * a
				+
				atmosphere.mie_scattering * mie_density *
				b * domega_i);
		}
	}

	retValue = rayleigh_mie;
	);
}

String SkyCommonFS::ComputeMultipleScattering(RadianceSpectrum& retValue, 
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const ScatteringDensityTexture& scattering_density_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground) {
	
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);

	// Number of intervals for the numerical integration.
	const int SAMPLE_COUNT = 50;
	// The integration step, i.e. the length of each integration interval.
	Length dx;
	DistanceToNearestAtmosphereBoundary(dx,
		atmosphere, r, mu, ray_r_mu_intersects_ground);
	dx = dx / Number(SAMPLE_COUNT);
	// Integration loop.
	RadianceSpectrum rayleigh_mie_sum =
		RadianceSpectrum(0.0 * watt_per_square_meter_per_sr_per_nm);
	for (int i = 0; i <= SAMPLE_COUNT; ++i) {
		Length d_i = Number(i) * dx;

		// The r, mu and mu_s parameters at the current integration point (see the
		// single scattering section for a detailed explanation).
		Length r_i =
			ClampRadius(atmosphere, sqrt(d_i * d_i + Number(2.0) * r * mu * d_i + r * r));
		Number mu_i = ClampCosine((r * mu + d_i) / r_i);
		Number mu_s_i = ClampCosine((r * mu_s + d_i * nu) / r_i);

		// The Rayleigh and Mie multiple scattering at the current sample point.
		RadianceSpectrum rayleigh_mie_i;
		RadianceSpectrum a;
		RadianceSpectrum b;
		GetScatteringAbstract(a,
			atmosphere, scattering_density_texture, r_i, mu_i, mu_s_i, nu,
			ray_r_mu_intersects_ground);
		GetTransmittance(b,
			atmosphere, transmittance_texture, r, mu, d_i,
			ray_r_mu_intersects_ground);
		rayleigh_mie_i = a * b * dx;
		// Sample weight (from the trapezoidal rule).
		Number weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
		rayleigh_mie_sum += rayleigh_mie_i * weight_i;
	}

	retValue = rayleigh_mie_sum;
	);
}

String SkyCommonFS::ComputeScatteringDensityTexture(RadianceDensitySpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const ReducedScatteringTexture& single_rayleigh_scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	const ScatteringTexture& multiple_scattering_texture,
	const IrradianceTexture& irradiance_texture,
	const Float3& frag_coord, int scattering_order) {
	return UnifyShaderCode(
		Length r;
	Number mu;
	Number mu_s;
	Number nu;
	bool ray_r_mu_intersects_ground;
	GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	ComputeScatteringDensity(retValue, atmosphere, transmittance_texture,
		single_rayleigh_scattering_texture, single_mie_scattering_texture,
		multiple_scattering_texture, irradiance_texture, r, mu, mu_s, nu,
		scattering_order);
	);
}

String SkyCommonFS::ComputeMultipleScatteringTexture(RadianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const ScatteringDensityTexture& scattering_density_texture,
	const Float3& frag_coord, Number& nu) {
	return UnifyShaderCode(
		Length r;
	Number mu;
	Number mu_s;
	bool ray_r_mu_intersects_ground;
	GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	ComputeMultipleScattering(retValue, atmosphere, transmittance_texture,
		scattering_density_texture, r, mu, mu_s, nu,
		ray_r_mu_intersects_ground);
	);
}

String SkyCommonFS::ComputeDirectIrradiance(IrradianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	Length r, Number mu_s) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu_s >= -1.0 && mu_s <= 1.0);

	Number alpha_s = atmosphere.sun_angular_radius / rad;
	// Approximate average of the cosine factor mu_s over the visible fraction of
	// the Sun disc.
	Number average_cosine_factor =
		mu_s < -alpha_s ? 0.0 : (mu_s > alpha_s ? mu_s :
			(mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));
	DimensionlessSpectrum a;
	GetTransmittanceToTopAtmosphereBoundary(a,
		atmosphere, transmittance_texture, r, mu_s);
	retValue = atmosphere.solar_irradiance * a
		 * average_cosine_factor;
	);
}

String SkyCommonFS::ComputeIndirectIrradiance(IrradianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const ReducedScatteringTexture& single_rayleigh_scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	const ScatteringTexture& multiple_scattering_texture,
	Length r, Number mu_s, int scattering_order) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(scattering_order >= 1);

	const int SAMPLE_COUNT = 32;
	const Angle dphi = pi / Number(SAMPLE_COUNT);
	const Angle dtheta = pi / Number(SAMPLE_COUNT);

	IrradianceSpectrum result =
		IrradianceSpectrum(0.0 * watt_per_square_meter_per_nm);
	float3 omega_s = float3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);
	for (int j = 0; j < SAMPLE_COUNT / 2; ++j) {
		Angle theta = (Number(j) + 0.5) * dtheta;
		for (int i = 0; i < 2 * SAMPLE_COUNT; ++i) {
			Angle phi = (Number(i) + 0.5) * dphi;
			float3 omega =
				float3(float(cos(phi) * sin(theta)), float(sin(phi) * sin(theta)), float(cos(theta)));
			SolidAngle domega = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;

			Number nu = dot(omega, omega_s);
			float3 s;
			GetScatteringReduced(s, atmosphere, single_rayleigh_scattering_texture,
				single_mie_scattering_texture, multiple_scattering_texture,
				r, omega.z, mu_s, nu, false /* ray_r_theta_intersects_ground */,
				scattering_order);
			result = result + s * omega.z * domega;
		}
	}

	retValue = result;
	);
}

String SkyCommonFS::GetIrradianceTextureUvFromRMuS(Float2& retValue, const AtmosphereParameters& atmosphere, Length r, Number mu_s) {
	return UnifyShaderCode(
		assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	Number x_r = (r - atmosphere.bottom_radius) /
		(atmosphere.top_radius - atmosphere.bottom_radius);
	Number x_mu_s = mu_s * 0.5 + 0.5;
	Number a;
	Number b;
	GetTextureCoordFromUnitRange(a, x_mu_s, IRRADIANCE_TEXTURE_WIDTH);
	GetTextureCoordFromUnitRange(b, x_r, IRRADIANCE_TEXTURE_HEIGHT);
	retValue = float2(a, b);
	);
}

String SkyCommonFS::GetIrradiance(IrradianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const IrradianceTexture& irradiance_texture,
	Length r, Number mu_s) {
	return UnifyShaderCode(float2 uv;
	GetIrradianceTextureUvFromRMuS(uv, atmosphere, r, mu_s);
	retValue = IrradianceSpectrum(texture(irradiance_texture, uv));
	);
}

String SkyCommonFS::GetRMuSFromIrradianceTextureUv(const AtmosphereParameters& atmosphere,
	const Float2& uv, Length& r, Number& mu_s) {
	return UnifyShaderCode(
		assert(uv.x >= 0.0 && uv.x <= 1.0);
	assert(uv.y >= 0.0 && uv.y <= 1.0);
	Number x_mu_s;
	GetUnitRangeFromTextureCoord(x_mu_s, uv.x, IRRADIANCE_TEXTURE_WIDTH);
	Number x_r;
	GetUnitRangeFromTextureCoord(x_r, uv.y, IRRADIANCE_TEXTURE_HEIGHT);
	r = atmosphere.bottom_radius +
		x_r * (atmosphere.top_radius - atmosphere.bottom_radius);
	mu_s = ClampCosine(Number(2.0) * x_mu_s - Number(1.0));
	);
}

String SkyCommonFS::ComputeDirectIrradianceTexture(IrradianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const Float2& frag_coord) {

	return UnifyShaderCode(
		const float2 IRRADIANCE_TEXTURE_SIZE =
		float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
	Length r;
	Number mu_s;
	GetRMuSFromIrradianceTextureUv(
		atmosphere, frag_coord / IRRADIANCE_TEXTURE_SIZE, r, mu_s);
	ComputeDirectIrradiance(retValue, atmosphere, transmittance_texture, r, mu_s);
	);
}

String SkyCommonFS::ComputeIndirectIrradianceTexture(IrradianceSpectrum& retValue, 
	const AtmosphereParameters& atmosphere,
	const ReducedScatteringTexture& single_rayleigh_scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	const ScatteringTexture& multiple_scattering_texture,
	const Float2& frag_coord, int scattering_order) {
	return UnifyShaderCode(
		const float2 IRRADIANCE_TEXTURE_SIZE =
		float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
		Length r;
	Number mu_s;
	GetRMuSFromIrradianceTextureUv(
		atmosphere, frag_coord / IRRADIANCE_TEXTURE_SIZE, r, mu_s);
	ComputeIndirectIrradiance(retValue, atmosphere,
		single_rayleigh_scattering_texture, single_mie_scattering_texture,
		multiple_scattering_texture, r, mu_s, scattering_order);
	);
}

String SkyCommonFS::GetCombinedScattering(IrradianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const ReducedScatteringTexture& scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground,
	IrradianceSpectrum& single_mie_scattering) {
	return UnifyShaderCode(
		float4 uvwz;
	GetScatteringTextureUvwzFromRMuMuSNu(uvwz,
		atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	Number tex_coord_x = uvwz.x * Number(SCATTERING_TEXTURE_NU_SIZE - 1);
	Number tex_x = floor(tex_coord_x);
	Number lp = tex_coord_x - tex_x;
	float3 uvw0 = float3((tex_x + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	IrradianceSpectrum scattering = IrradianceSpectrum(
		texture(scattering_texture, uvw0).xyz * (Number(1.0) - lp) +
		texture(scattering_texture, uvw1).xyz * lp);
	single_mie_scattering = IrradianceSpectrum(
		texture(single_mie_scattering_texture, uvw0).xyz * (Number(1.0) - lp) +
		texture(single_mie_scattering_texture, uvw1).xyz * lp);
	retValue = scattering;
	);
}

String SkyCommonFS::GetSkyRadiance(RadianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const ReducedScatteringTexture& scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	Position& camera, const Direction& view_ray, Length shadow_length,
	const Direction& sun_direction, DimensionlessSpectrum& transmittance) {
	return UnifyShaderCode(
		// Compute the distance to the top atmosphere boundary along the view ray,
		// assuming the viewer is in space (or NaN if the view ray does not intersect
		// the atmosphere).
		while (true) {
			Length r = length(camera);
			Length rmu = dot(camera, view_ray);
			Length distance_to_top_atmosphere_boundary = -rmu -
				sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
			// If the viewer is in space and the view ray intersects the atmosphere, move
			// the viewer to the top atmosphere boundary (along the view ray):
			if (distance_to_top_atmosphere_boundary > 0.0 * m) {
				camera = camera + view_ray * distance_to_top_atmosphere_boundary;
				r = atmosphere.top_radius;
				rmu += distance_to_top_atmosphere_boundary;
			} else if (r > atmosphere.top_radius) {
				// If the view ray does not intersect the atmosphere, simply return 0.
				transmittance = DimensionlessSpectrum(1.0);
				retValue = RadianceSpectrum(0.0 * watt_per_square_meter_per_sr_per_nm);
				break;
			}
			// Compute the r, mu, mu_s and nu parameters needed for the texture lookups.
			Number mu = rmu / r;
			Number mu_s = dot(camera, sun_direction) / r;
			Number nu = dot(view_ray, sun_direction);
			bool ray_r_mu_intersects_ground;
			RayIntersectsGround(ray_r_mu_intersects_ground, atmosphere, r, mu);

			if (ray_r_mu_intersects_ground) {
				GetTransmittanceToTopAtmosphereBoundary(transmittance,
					atmosphere, transmittance_texture, r, mu);
			} else {
				transmittance = DimensionlessSpectrum(0.0);
			}

			IrradianceSpectrum single_mie_scattering;
			IrradianceSpectrum scattering;
			if (shadow_length == 0.0 * m) {
				GetCombinedScattering(scattering,
					atmosphere, scattering_texture, single_mie_scattering_texture,
					r, mu, mu_s, nu, ray_r_mu_intersects_ground,
					single_mie_scattering);
			} else {
				// Case of light shafts (shadow_length is the total length noted l in our
				// paper): we omit the scattering between the camera and the point at
				// distance l, by implementing Eq. (18) of the paper (shadow_transmittance
				// is the T(x,x_s) term, scattering is the S|x_s=x+lv term).
				Length d = shadow_length;
				Length r_p =
					ClampRadius(atmosphere, Number(sqrt(d * d + Number(2.0) * r * mu * d + r * r)));
				Number mu_p = (r * mu + d) / r_p;
				Number mu_s_p = (r * mu_s + d * nu) / r_p;

				GetCombinedScattering(scattering,
					atmosphere, scattering_texture, single_mie_scattering_texture,
					r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
					single_mie_scattering);
				DimensionlessSpectrum shadow_transmittance;
				GetTransmittance(shadow_transmittance, atmosphere, transmittance_texture,
					r, mu, shadow_length, ray_r_mu_intersects_ground);
				scattering = scattering * shadow_transmittance;
				single_mie_scattering = single_mie_scattering * shadow_transmittance;
			}

			InverseSolidAngle a;
			InverseSolidAngle b;
			RayleighPhaseFunction(a, nu);
			MiePhaseFunction(b, atmosphere.mie_phase_function_g, nu);
			retValue = scattering * a + single_mie_scattering * b;
			break;
		}
	);
}

String SkyCommonFS::GetSkyRadianceToPoint(RadianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const ReducedScatteringTexture& scattering_texture,
	const ReducedScatteringTexture& single_mie_scattering_texture,
	Position& camera, const Position& point, Length shadow_length,
	const Direction& sun_direction, DimensionlessSpectrum& transmittance) {
	return UnifyShaderCode(
		// Compute the distance to the top atmosphere boundary along the view ray,
		// assuming the viewer is in space (or NaN if the view ray does not intersect
		// the atmosphere).
		Direction view_ray = normalize(point - camera);
	Length r = length(camera);
	Length rmu = dot(camera, view_ray);
	Length distance_to_top_atmosphere_boundary = -rmu -
		sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
	// If the viewer is in space and the view ray intersects the atmosphere, move
	// the viewer to the top atmosphere boundary (along the view ray):
	if (distance_to_top_atmosphere_boundary > 0.0 * m) {
		camera = camera + view_ray * distance_to_top_atmosphere_boundary;
		r = atmosphere.top_radius;
		rmu += distance_to_top_atmosphere_boundary;
	}

	// Compute the r, mu, mu_s and nu parameters for the first texture lookup.
	Number mu = rmu / r;
	Number mu_s = dot(camera, sun_direction) / r;
	Number nu = dot(view_ray, sun_direction);
	Length d = length(point - camera);
	bool ray_r_mu_intersects_ground;
	RayIntersectsGround(ray_r_mu_intersects_ground, atmosphere, r, mu);
	GetTransmittance(transmittance, atmosphere, transmittance_texture,
		r, mu, d, ray_r_mu_intersects_ground);

	IrradianceSpectrum single_mie_scattering;
	IrradianceSpectrum scattering;
	GetCombinedScattering(scattering,
		atmosphere, scattering_texture, single_mie_scattering_texture,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground,
		single_mie_scattering);

	// Compute the r, mu, mu_s and nu parameters for the second texture lookup.
	// If shadow_length is not 0 (case of light shafts), we want to ignore the
	// scattering along the last shadow_length meters of the view ray, which we
	// do by subtracting shadow_length from d (this way scattering_p is equal to
	// the S|x_s=x_0-lv term in Eq. (17) of our paper).
	d = max(d - shadow_length, 0.0 * m);
	Length r_p = ClampRadius(atmosphere, sqrt(d * d + Number(2.0) * r * mu * d + r * r));
	Number mu_p = (r * mu + d) / r_p;
	Number mu_s_p = (r * mu_s + d * nu) / r_p;

	IrradianceSpectrum single_mie_scattering_p;
	IrradianceSpectrum scattering_p;
	GetCombinedScattering(scattering_p,
		atmosphere, scattering_texture, single_mie_scattering_texture,
		r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
		single_mie_scattering_p);

	// Combine the lookup results to get the scattering between camera and point.
	DimensionlessSpectrum shadow_transmittance = transmittance;
	if (shadow_length > 0.0 * m) {
		// This is the T(x,x_s) term in Eq. (17) of our paper, for light shafts.
		GetTransmittance(shadow_transmittance, atmosphere, transmittance_texture,
			r, mu, d, ray_r_mu_intersects_ground);
	}
	scattering = scattering - shadow_transmittance * scattering_p;
	single_mie_scattering =
		single_mie_scattering - shadow_transmittance * single_mie_scattering_p;

	// Hack to avoid rendering artifacts when the sun is below the horizon.
	single_mie_scattering = single_mie_scattering *
		smoothstep(Number(0.0), Number(0.01), mu_s);
	InverseSolidAngle a;
	InverseSolidAngle b;
	RayleighPhaseFunction(a, nu);
	MiePhaseFunction(b, atmosphere.mie_phase_function_g, nu);
	retValue = scattering * a + single_mie_scattering * b;
	);
}

String SkyCommonFS::GetSunAndSkyIrradiance(IrradianceSpectrum& retValue,
	const AtmosphereParameters& atmosphere,
	const TransmittanceTexture& transmittance_texture,
	const IrradianceTexture& irradiance_texture,
	const Position& point, const Direction& normal, const Direction& sun_direction,
	IrradianceSpectrum& sky_irradiance) {
	return UnifyShaderCode(
		Length r = length(point);
	Number mu_s = dot(point, sun_direction) / r;

	// Indirect irradiance (approximated if the surface is not horizontal).
	GetIrradiance(sky_irradiance, atmosphere, irradiance_texture, r, mu_s);
	sky_irradiance = sky_irradiance * 
		(Number(1.0) + dot(normal, point) / r) * Number(0.5);

	// Direct irradiance.
	IrradianceSpectrum sun;
	GetTransmittanceToSun(sun,
		atmosphere, transmittance_texture, r, mu_s);
	retValue = atmosphere.solar_irradiance * sun * max(dot(normal, sun_direction), Number(0.0));
	);
}

SkyCommonFS::SkyCommonFS() {
	paramBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;

	protoTransmittanceTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	protoAbstractSpectrumTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	protoReducedScatteringTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	protoScatteringTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	protoIrradianceTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	protoScatteringDensityTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;

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
}

TObject<IReflect>& SkyCommonFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(TRANSMITTANCE_TEXTURE_WIDTH)[BindConst<float>(TRANSMITTANCE_TEXTURE_WIDTH)];
		ReflectProperty(TRANSMITTANCE_TEXTURE_HEIGHT)[BindConst<float>(TRANSMITTANCE_TEXTURE_HEIGHT)];
		ReflectProperty(SCATTERING_TEXTURE_R_SIZE)[BindConst<float>(SCATTERING_TEXTURE_R_SIZE)];
		ReflectProperty(SCATTERING_TEXTURE_MU_SIZE)[BindConst<float>(SCATTERING_TEXTURE_MU_SIZE)];
		ReflectProperty(SCATTERING_TEXTURE_MU_S_SIZE)[BindConst<float>(SCATTERING_TEXTURE_MU_S_SIZE)];
		ReflectProperty(SCATTERING_TEXTURE_NU_SIZE)[BindConst<float>(SCATTERING_TEXTURE_NU_SIZE)];
		ReflectProperty(SCATTERING_TEXTURE_WIDTH)[BindConst<float>(SCATTERING_TEXTURE_WIDTH)];
		ReflectProperty(SCATTERING_TEXTURE_HEIGHT)[BindConst<float>(SCATTERING_TEXTURE_HEIGHT)];
		ReflectProperty(SCATTERING_TEXTURE_DEPTH)[BindConst<float>(SCATTERING_TEXTURE_DEPTH)];
		ReflectProperty(IRRADIANCE_TEXTURE_WIDTH)[BindConst<float>(IRRADIANCE_TEXTURE_WIDTH)];
		ReflectProperty(IRRADIANCE_TEXTURE_HEIGHT)[BindConst<float>(IRRADIANCE_TEXTURE_HEIGHT)];

		ReflectProperty(paramBuffer);
		ReflectProperty(atmosphere)[paramBuffer];
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(DistanceToTopAtmosphereBoundary)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(DistanceToBottomAtmosphereBoundary)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(RayIntersectsGround)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(GetLayerDensity)[BindFunction()][MetaParameter("retValue")][MetaParameter("layer")][MetaParameter("altitude")];
		ReflectMethod(GetProfileDensity)[BindFunction()][MetaParameter("retValue")][MetaParameter("profile")][MetaParameter("altitude")];
		ReflectMethod(ComputeOpticalLengthToTopAtmosphereBoundary)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("profile")][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(ComputeTransmittanceToTopAtmosphereBoundary)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(GetTextureCoordFromUnitRange)[BindFunction()][MetaParameter("retValue")][MetaParameter("u")][MetaParameter("texture_size")];
		ReflectMethod(GetUnitRangeFromTextureCoord)[BindFunction()][MetaParameter("retValue")][MetaParameter("u")][MetaParameter("texture_size")];
		ReflectMethod(GetTransmittanceTextureUvFromRMu)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(GetRMuFromTransmittanceTextureUv)[BindFunction()][MetaParameter("atmosphere")][MetaParameter("uv")][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(ComputeTransmittanceToTopAtmosphereBoundaryTexture)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("frag_coord")];
		ReflectMethod(GetTransmittanceToTopAtmosphereBoundary)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("r")][MetaParameter("mu")];
		ReflectMethod(GetTransmittance)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("d")][MetaParameter("ray_r_mu_intersects_ground")];
		ReflectMethod(GetTransmittanceToSun)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("r")][MetaParameter("mu_s")];
		ReflectMethod(ComputeSingleScatteringIntegrand)[BindFunction()]
			[MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("d")][MetaParameter("ray_r_mu_intersects_ground")][MetaParameter("rayleigh")][MetaParameter("mie")];
		ReflectMethod(DistanceToNearestAtmosphereBoundary)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu")][MetaParameter("ray_r_mu_intersects_ground")];
		ReflectMethod(ComputeSingleScattering)[BindFunction()][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")][MetaParameter("rayleigh")][MetaParameter("mie")];
		ReflectMethod(RayleighPhaseFunction)[BindFunction()][MetaParameter("retValue")][MetaParameter("nu")];
		ReflectMethod(MiePhaseFunction)[BindFunction()][MetaParameter("retValue")][MetaParameter("g")][MetaParameter("nu")];
		ReflectMethod(GetScatteringTextureUvwzFromRMuMuSNu)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")];
		ReflectMethod(GetRMuMuSNuFromScatteringTextureUvwz)[BindFunction()][MetaParameter("atmosphere")][MetaParameter("uvwz")][MetaParameter("r")][MetaParameter("mu")][MetaParameter(" mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")];
		ReflectMethod(GetRMuMuSNuFromScatteringTextureFragCoord)[BindFunction()][MetaParameter("atmosphere")][MetaParameter("frag_coord")][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")];
		ReflectMethod(ComputeSingleScatteringTexture)[BindFunction()][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("frag_coord")][MetaParameter("rayleigh")][MetaParameter("mie")];
		ReflectMethod(GetScatteringAbstract)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("scattering_texture", &protoScatteringTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")];
		ReflectMethod(GetScatteringReduced)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("single_rayleigh_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("multiple_scattering_texture", &protoScatteringTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")][MetaParameter("scattering_order")];
		ReflectMethod(GetIrradianceTextureUvFromRMuS)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("r")][MetaParameter("mu_s")];
		ReflectMethod(GetIrradiance)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("irradiance_texture", &protoIrradianceTexture)][MetaParameter("r")][MetaParameter("mu_s")];
		ReflectMethod(ComputeScatteringDensity)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("single_rayleigh_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("multiple_scattering_texture", &protoScatteringTexture)][MetaParameter("irradiance_texture", &protoIrradianceTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("scattering_order")];
		ReflectMethod(ComputeMultipleScattering)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("scattering_density_texture", &protoScatteringDensityTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")];
		ReflectMethod(ComputeScatteringDensityTexture)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("single_rayleigh_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("multiple_scattering_texture", &protoScatteringTexture)][MetaParameter("irradiance_texture", &protoIrradianceTexture)][MetaParameter("frag_coord")][MetaParameter("scattering_order")];
		ReflectMethod(ComputeMultipleScatteringTexture)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("scattering_density_texture", &protoScatteringDensityTexture)][MetaParameter("frag_coord")][MetaParameter("nu")];
		ReflectMethod(ComputeDirectIrradiance)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("r")][MetaParameter("mu_s")];
		ReflectMethod(ComputeIndirectIrradiance)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("single_rayleigh_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("multiple_scattering_texture", &protoScatteringTexture)][MetaParameter("r")][MetaParameter("mu_s")][MetaParameter("scattering_order")];
		ReflectMethod(GetRMuSFromIrradianceTextureUv)[BindFunction()][MetaParameter("atmosphere")][MetaParameter("uv")][MetaParameter("r")][MetaParameter("mu_s")];
		ReflectMethod(ComputeDirectIrradianceTexture)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("frag_coord")];
		ReflectMethod(ComputeIndirectIrradianceTexture)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("single_rayleigh_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("multiple_scattering_texture", &protoScatteringTexture)][MetaParameter("frag_coord")][MetaParameter("scattering_order")];
		ReflectMethod(GetCombinedScattering)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("scattering_texture", &protoScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("r")][MetaParameter("mu")][MetaParameter("mu_s")][MetaParameter("nu")][MetaParameter("ray_r_mu_intersects_ground")][MetaParameter("single_mie_scattering")];
		ReflectMethod(GetSkyRadiance)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("scattering_texture", &protoScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("camera")][MetaParameter("view_ray")][MetaParameter("shadow_length")][MetaParameter("sun_direction")][MetaParameter("transmittance")];
		ReflectMethod(GetSkyRadianceToPoint)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("scattering_texture", &protoScatteringTexture)][MetaParameter("single_mie_scattering_texture", &protoReducedScatteringTexture)][MetaParameter("camera")][MetaParameter("point")][MetaParameter("shadow_length")][MetaParameter("sun_direction")][MetaParameter("transmittance")];
		ReflectMethod(GetSunAndSkyIrradiance)[BindFunction()][MetaParameter("retValue")][MetaParameter("atmosphere")][MetaParameter("transmittance_texture", &protoTransmittanceTexture)][MetaParameter("irradiance_texture", &protoIrradianceTexture)][MetaParameter("point")][MetaParameter("normal")][MetaParameter("sun_direction")][MetaParameter("sky_irradiance")];
	}

	return *this;
}
