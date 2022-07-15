// SkyComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Model/ModelComponent.h"
#include "../../../SnowyStream/Resource/Shaders/SkyCommonDef.h"

namespace PaintsNow {
	class SkyComponent : public TAllocatedTiny<SkyComponent, ModelComponent> {
	public:
		SkyComponent(const TShared<MeshResource>& meshResource, const TShared<BatchComponent>& batchComponent);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		size_t ReportGraphicMemoryUsage() const override;
		void SetSkyTexture(const TShared<TextureResource>& texture);
		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;

	protected:
		void UpdateMaterial();
		void InitializeAtmosphereParameters(const std::vector<double>& wavelengths,
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
			unsigned int num_precomputed_wavelengths);

	protected:
		class GlobalParameters : public TReflected<GlobalParameters, IReflectObjectComplex> {
		public:
			GlobalParameters();
			TObject<IReflect>& operator () (IReflect& reflect) override;

			int TRANSMITTANCE_TEXTURE_WIDTH;
			int TRANSMITTANCE_TEXTURE_HEIGHT;

			int SCATTERING_TEXTURE_R_SIZE;
			int SCATTERING_TEXTURE_MU_SIZE;
			int SCATTERING_TEXTURE_MU_S_SIZE;
			int SCATTERING_TEXTURE_NU_SIZE;

			int SCATTERING_TEXTURE_WIDTH;
			int SCATTERING_TEXTURE_HEIGHT;
			int SCATTERING_TEXTURE_DEPTH;

			int IRRADIANCE_TEXTURE_WIDTH;
			int IRRADIANCE_TEXTURE_HEIGHT;

			double MAX_LUMINOUS_EFFICACY;
			Float3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
			Float3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
		};

		GlobalParameters globalParameters;
		AtmosphereParameters atmosphereParameters;
		TShared<TextureResource> transmittance_texture;
		TShared<TextureResource> scattering_texture;
		TShared<TextureResource> irradiance_texture;

		TShared<TextureResource> skyTexture;
	};
}
