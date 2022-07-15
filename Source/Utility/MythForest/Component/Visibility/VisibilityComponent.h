// VisibilityComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../Explorer/SpaceTraversal.h"
#include "../Explorer/CameraCuller.h"
#include "../../../../Core/Template/TBuffer.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/Passes/ConstMapPass.h"
#include "../Stream/StreamComponent.h"

namespace PaintsNow {
	class SpaceComponent;
	class TransformComponent;
	class RenderableComponent;

	struct VisibilityComponentConfig {
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
			std::vector<Bytes> instancedData;
			IRender::Resource::QuickDrawCallDescription drawCallDescription;
			uint32_t instanceCount;
		};

		class MergedInstanceGroup {
		public:
			MergedInstanceGroup();

			InstanceGroup* groupPrototype;
			std::vector<Bytes> mergedInstanceData;
			uint32_t mergedInstanceCount;
		};

		class CaptureData : public FrustrumCuller {};

		class Cache {
		public:
			Int3 intPosition;
			Bytes payload;
		};

		class TaskData;
		class Cell : public TAllocatedTiny<Cell, SharedTiny>, public Cache {
		public:
			enum {
				FACE_COUNT = 6,
				ALL_FACE_MASK = (1 << FACE_COUNT) - 1
			};

			Cell();

			std::atomic<uint32_t> dispatched; // bitmask
			std::atomic<uint32_t> finished; // bitmask
		};

		class TaskData {
		public:
			enum {
				STATUS_IDLE,
				STATUS_START,
				STATUS_DISPATCHED,
				STATUS_ASSEMBLING,
				STATUS_ASSEMBLED,
				STATUS_BAKING,
				STATUS_BAKED,
				STATUS_POSTPROCESS,
			};

			TaskData() : status(STATUS_IDLE), pendingCount(0), faceIndex(0), pendingResourceCount(0), renderQueue(nullptr), renderTarget(nullptr) {}
			bool Continue() const { return pendingResourceCount == 0; }

			class WarpData {
			public:
				typedef std::unordered_map<InstanceKey, InstanceGroup> InstanceGroupMap;
				InstanceGroupMap instanceGroups;
				BytesCache bytesCache;
			};

			uint32_t pendingCount;
			uint32_t status;
			uint32_t pendingResourceCount;
			uint32_t faceIndex;
			TShared<Cell> cell;
			IRender::Queue* renderQueue;
			IRender::Resource* renderTarget;
			TShared<TextureResource> texture;
			Bytes data;
			PerspectiveCamera camera;
			std::vector<WarpData> warpData;
			BytesCache globalBytesCache;
#if defined(_MSC_VER) && _MSC_VER <= 1200
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup> MergedInstanceGroupMap;
#else
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup, std::hash<InstanceKey>, std::equal_to<InstanceKey>, TCacheAllocator<std::pair<const InstanceKey, MergedInstanceGroup> > > MergedInstanceGroupMap;
#endif
		};
	};

	class VisibilityComponent : public TAllocatedTiny<VisibilityComponent, UniqueComponent<Component> >, public SpaceTraversal<VisibilityComponent, VisibilityComponentConfig> {
	public:
		friend class SpaceTraversal<VisibilityComponent, VisibilityComponentConfig>;
		VisibilityComponent(const TShared<StreamComponent>& streamComponent);
		enum {
			VISIBILITYCOMPONENT_PARALLEL = COMPONENT_CUSTOM_BEGIN,
			VISIBILITYCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		typedef VisibilityComponentConfig::InstanceKey InstanceKey;
		typedef VisibilityComponentConfig::InstanceGroup InstanceGroup;
		typedef VisibilityComponentConfig::MergedInstanceGroup MergedInstanceGroup;
		typedef VisibilityComponentConfig::Cache Cache;
		typedef VisibilityComponentConfig::Cell Cell;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(Engine& engine, Entity* entity) final;
		void Uninitialize(Engine& engine, Entity* entity) final;
		FLAG GetEntityFlagMask() const final;
		void DispatchEvent(Event& event, Entity* entity) final;
		Entity* GetHostEntity() const final;

		const Bytes& QuerySample(Engine& engine, const Float3& position);
		static bool IsVisible(const Bytes& sample, TransformComponent* transformComponent);

		void Setup(Engine& engine, float distance, const Float3& gridSize, uint32_t taskCount, uint32_t pixelKillThreshold, const UShort2& resolution);

	protected:
		void TickRender(Engine& engine);
		void RoutineTickTasks(Engine& engine);
		void ResolveTasks(Engine& engine);
		void DispatchTasks(Engine& engine);
		void PostProcess(TaskData& task);

		void CoTaskAssembleTask(Engine& engine, TaskData& task, const TShared<VisibilityComponent>& selfHolder);
		void SetupIdentities(Engine& engine, Entity* hostEntity);
		void RoutineSetupIdentities(Engine& engine, Entity* hostEntity);
		void CollectRenderableComponent(Engine& engine, TaskData& task, RenderableComponent* renderableComponent, WorldInstanceData& instanceData, uint32_t identity);
		void CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity);
		void CompleteCollect(Engine& engine, TaskData& task);

		TShared<SharedTiny> StreamLoadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context);
		TShared<SharedTiny> StreamUnloadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context);

	protected:
		TShared<StreamComponent> streamComponent;
		Entity* hostEntity;

		Float3 gridSize;
		float viewDistance;
		uint32_t taskCount;
		std::atomic<uint32_t> pendingSetupCount;
		UShort2 resolution;
		std::atomic<uint32_t> maxVisIdentity;
		uint16_t activeCellCacheIndex;
		uint16_t pixelKillThreshold;
		Cache cellCache[8];
		TShared<TObjectAllocator<Cell> > cellAllocator;

		// Runtime Baker
		IRender::Queue* renderQueue;
		IRender::Resource* depthStencilResource;
		IRender::Resource* stateResource;
		IThread::Lock* collectLock;
		TShared<ShaderResourceImpl<ConstMapPass> > pipeline;
		std::vector<TaskData> tasks;
		std::vector<TShared<Cell> > bakePoints;
	};
}
