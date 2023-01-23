// PhaseComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../../../Core/Template/TCache.h"
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../SnowyStream/Resource/Passes/MultiHashTracePass.h"
#include "../../../SnowyStream/Resource/Passes/MultiHashSetupPass.h"
#include "../../../SnowyStream/Resource/Passes/ConstMapPass.h"
#include "../Explorer/CameraCuller.h"
#include "../Explorer/SpaceTraversal.h"
#include "../RenderFlow/RenderFlowComponent.h"
#include "../RenderFlow/RenderPort/RenderPortPhaseLightView.h"

namespace PaintsNow {
	// Phase Rendering
	// 1. Render shadow map from each light
	// 2. Render direct light distribution map with multi-pass lighting and shadow map
	// 3. Generate multiple phases and rasterize g-buffer infos
	// 4. Bounce radiance between phases.
	// 5. Gathering irradiance from phases.
	class SpaceComponent;
	class RenderableComponent;

	struct PhaseComponentConfig {
		class CaptureData : public FrustrumCuller {};

		class WorldGlobalData : public TReflected<WorldGlobalData, PassBase::PartialData> {
		public:
			WorldGlobalData();

			TObject<IReflect>& operator () (IReflect& reflect) override;
			MatrixFloat4x4 viewProjectionMatrix;
			MatrixFloat4x4 viewMatrix;
			MatrixFloat4x4 lightReprojectionMatrix;
			Float4 lightColor;
			Float4 lightPosition;
			Float2 invScreenSize;
			IShader::BindTexture noiseTexture;
			IShader::BindTexture lightDepthTexture;
		};

		class WorldInstanceData : public TReflected<WorldInstanceData, PassBase::PartialData> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override;

			MatrixFloat4x4 worldMatrix;
			Float3Pair boundingBox;
			Float4 instancedColor;
		};

		typedef uint32_t InstanceKey;

		class InstanceGroup {
		public:
			InstanceGroup();
			void Cleanup();

			PassBase::PartialUpdater instanceUpdater;
			IRender::Resource* renderStateResource;
			IRender::Resource* drawCallResource;
			std::vector<Bytes> instancedData;
			IRender::Resource::QuickDrawCallDescription drawCallDescription;
			TShared<ShaderResource> shaderResource;
			uint32_t instanceCount;
		};

		class MergedInstanceGroup {
		public:
			MergedInstanceGroup();

			InstanceGroup* groupPrototype;
			std::vector<Bytes> mergedInstanceData;
			uint32_t mergedInstanceCount;
		};

		class TaskData {
		public:
			class_aligned(CPU_CACHELINE_SIZE) WarpData {
			public:
				typedef std::unordered_map<InstanceKey, InstanceGroup> InstanceGroupMap;
				BytesCache bytesCache;
				InstanceGroupMap instanceGroups;
			};

			class GlobalBufferItem {
			public:
				PassBase::PartialUpdater globalUpdater;
				std::vector<IRender::Resource::DrawCallDescription::BufferRange> buffers;
				std::vector<IRender::Resource*> textures;
			};

			TaskData();
			virtual ~TaskData();
			void Cleanup(IRender& render);
			void Destroy(IRender& render);
			bool Continue() const { return pendingResourceCount == 0; }

			enum {
				STATUS_IDLE,
				STATUS_START,
				STATUS_DISPATCHED,
				STATUS_ASSEMBLING,
				STATUS_ASSEMBLED,
				STATUS_DOWNLOADED,
				STATUS_BAKED
			};

			std::vector<WarpData> warpData;
			WorldGlobalData worldGlobalData;
			std::vector<KeyValue<ShaderResource*, GlobalBufferItem> > worldGlobalBufferMap;
			BytesCache globalBytesCache;
#if defined(_MSC_VER) && _MSC_VER <= 1200
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup> MergedInstanceGroupMap;
#else
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup, std::hash<InstanceKey>, std::equal_to<InstanceKey>, TCacheAllocator<std::pair<const InstanceKey, MergedInstanceGroup> > > MergedInstanceGroupMap;
#endif
			MergedInstanceGroupMap mergedInstancedGroupMap;

			uint32_t status;
			uint32_t pendingCount;
			uint32_t pendingResourceCount;
			IRender::Queue* renderQueue;
			IRender::Resource* renderTarget;
			ShaderResource* pipeline;
			PerspectiveCamera camera;
			std::vector<TShared<TextureResource> > textures;
		};
	};

	class PhaseComponent : public TAllocatedTiny<PhaseComponent, Component>, public SpaceTraversal<PhaseComponent, PhaseComponentConfig> {
	public:
		friend class SpaceTraversal<PhaseComponent, PhaseComponentConfig>;
		typedef PhaseComponentConfig::InstanceKey InstanceKey;
		typedef PhaseComponentConfig::InstanceGroup InstanceGroup;
		typedef PhaseComponentConfig::MergedInstanceGroup MergedInstanceGroup;

		PhaseComponent(const TShared<RenderFlowComponent>& renderFlowComponent, const String& lightPhaseViewName);
		~PhaseComponent() override;

		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void DispatchEvent(Event& event, Entity* entity) override;
		Tiny::FLAG GetEntityFlagMask() const override;

		void BindRootEntity(Engine& engine, Entity* entity);
		void Update(Engine& engine, const Float3& position);
		void Step(Engine& engine, uint32_t bounceCount);
		void Resample(Engine& engine, uint32_t phaseCount);
		void Setup(Engine& engine, uint32_t phaseCount, uint32_t taskCount, const Float3& range, const UShort2& resolution);
		void SetDebugMode(const String& debugPath);

		class Phase : public RenderPortPhaseLightView::PhaseInfo {
		public:
			Phase();
			TShared<TextureResource> baseColorOcclusion;
			TShared<TextureResource> normalRoughnessMetallic;
			TShared<TextureResource> noiseTexture;
			uint32_t shadowIndex;

			// for tracing
			TShared<ShaderResourceImpl<MultiHashTracePass> > tracePipeline;
			IRender::Resource::DrawCallDescription drawCallDescription;
			IRender::Resource* drawCallResource;
			PerspectiveCamera camera;
			std::vector<IRender::Resource*> uniformBuffers;
		};

		const std::vector<Phase>& GetPhases() const;

	protected:
		void TickRender(Engine& engine);
		void ResolveTasks(Engine& engine);
		void DispatchTasks(Engine& engine);
		void UpdateRenderFlow(Engine& engine);
		void RoutineWriteDebugTexture(Engine& engine, uint32_t index, Bytes& data, const TShared<TextureResource>& texture);

		struct LightElement {
			Float4 position;
			Float4 colorAttenuation;
		};

		struct LightConfig {
			struct WorldInstanceData {
				MatrixFloat4x4 worldMatrix;
				Float3Pair boundingBox;
			};

			class_aligned(CPU_CACHELINE_SIZE) WarpData {
			public:
				std::vector<LightElement> lightElements;
			};

			class TaskData {
			public:
				TaskData(uint32_t warpCount);
				bool Continue() const { return pendingResourceCount == 0; }

				uint32_t pendingResourceCount;
				uint32_t pendingCount;
				std::vector<WarpData> warpData;
			};

			struct CaptureData {
				inline bool operator () (const Float3Pair& box) const {
					return true;
				}
			};
		};

		class Shadow {
		public:
			MatrixFloat4x4 viewMatrix;
			MatrixFloat4x4 projectionMatrix;
			TShared<TextureResource> shadow;
			LightElement lightElement;
		};

		struct UpdatePointSetup {
			uint32_t phaseIndex;
			uint32_t shadowIndex;
		};

		struct UpdatePointShadow {
			uint32_t shadowIndex;
		};

		struct UpdatePointBounce {
			uint32_t fromPhaseIndex;
			uint32_t toPhaseIndex;
		};

		void CoTaskAssembleTaskSetup(Engine& engine, TaskData& task, const UpdatePointSetup& bakePoint);
		void CoTaskAssembleTaskShadow(Engine& engine, TaskData& task, const UpdatePointShadow& bakePoint);
		void TaskAssembleTaskBounce(Engine& engine, TaskData& task, const UpdatePointBounce& bakePoint);
		void Collect(Engine& engine, TaskData& taskData, const MatrixFloat4x4& viewMatrix, const MatrixFloat4x4& worldMatrix);

		void CollectRenderableComponent(Engine& engine, TaskData& task, RenderableComponent* renderableComponent, WorldInstanceData& instanceData);
		void CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity);
		void CompleteCollect(Engine& engine, TaskData& task);

	protected:
		Entity* hostEntity;
		TShared<RenderFlowComponent> renderFlowComponent;
		String lightPhaseViewPortName;
		uint32_t maxTracePerTick;
		UShort2 resolution;
		Float3 range;
		IRender::Queue* renderQueue;
		IRender::Resource* stateSetupResource;
		IRender::Resource* statePostResource;
		IRender::Resource* stateShadowResource;

		TShared<ShaderResourceImpl<MultiHashTracePass> > tracePipeline;
		TShared<ShaderResourceImpl<MultiHashSetupPass> > setupPipeline;
		TShared<ShaderResourceImpl<ConstMapPass> > shadowPipeline;
		TShared<MeshResource> meshResource;

		std::vector<TaskData> tasks;
		std::vector<Phase> phases;
		std::vector<Shadow> shadows;
		std::stack<UpdatePointSetup> bakePointSetups;
		std::stack<UpdatePointShadow> bakePointShadows;
		std::stack<UpdatePointBounce> bakePointBounces;

	protected:
		class LightCollector : public SpaceTraversal<LightCollector, LightConfig> {
		public:
			friend class SpaceTraversal<LightCollector, LightConfig>;

			LightCollector(PhaseComponent* phaseComponent);
			void InvokeCollect(Engine& engine, Entity* entity);

		protected:
			void CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity);
			void CompleteCollect(Engine& engine, TaskData& task);
			PhaseComponent* phaseComponent;
		};

		void CompleteUpdateLights(Engine& engine, std::vector<LightElement>& lightElements);

		friend class LightCollector;
		LightCollector lightCollector;
		Entity* rootEntity;
		String debugPath;
	};
}

