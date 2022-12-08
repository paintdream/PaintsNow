// CameraComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/Template/TBuffer.h"
#include "../../../../Core/Template/TAlgorithm.h"
#include "../../../../Core/Template/TCache.h"
#include "../Animation/AnimationComponent.h"
#include "../Bridge/BridgeComponentModule.h"
#include "../RenderFlow/RenderFlowComponent.h"
#include "../RenderFlow/RenderPort/RenderPortCommandQueue.h"
#include "../RenderFlow/RenderPort/RenderPortLightSource.h"
#include "../Event/EventComponent.h"
#include "../Explorer/CameraCuller.h"
#include "../Explorer/SpaceTraversal.h"
#include <sstream>

namespace PaintsNow {
	class LightComponent;
	class EnvCubeComponent;
	class SpaceComponent;
	class RenderableComponent;

	struct CameraComponentConfig {
		typedef RenderPortLightSource::LightElement LightElement;
		typedef RenderPortLightSource::EnvCubeElement EnvCubeElement;

		class CaptureData : public FrustrumCuller {
		public:
			Bytes visData;
		};

		class WorldGlobalData : public TReflected<WorldGlobalData, PassBase::PartialData> {
		public:
			WorldGlobalData();

			TObject<IReflect>& operator () (IReflect& reflect) override;
			MatrixFloat4x4 viewProjectionMatrix;
			MatrixFloat4x4 viewMatrix;
			MatrixFloat4x4 projectionMatrix;
			MatrixFloat4x4 lastViewMatrix;
			MatrixFloat4x4 lastProjectionMatrix;
			MatrixFloat4x4 inverseViewMatrix; // inverse of viewMatrix
			MatrixFloat4x4 jitterMatrix;

			Float3 viewPosition;
			Float2 jitterOffset;
			float time;
			float tanHalfFov;
		};

		class WorldInstanceData : public TReflected<WorldInstanceData, PassBase::PartialData> {
		public:
			WorldInstanceData() : viewReference(0), fadeRatio(0) {}
			TObject<IReflect>& operator () (IReflect& reflect) override;

			MatrixFloat4x4 worldMatrix;
			Float3Pair boundingBox;
			float viewReference; // currently only works on Lod selection
			float fadeRatio;
			TShared<AnimationComponent> animationComponent;
		};

		typedef uint32_t InstanceKey;
		class InstanceGroup {
		public:
			InstanceGroup();
			void Cleanup();

			PassBase::PartialUpdater instanceUpdater;
			IRender::Resource* drawCallResource;
			IRender::Resource* renderStateResource;
			std::vector<Bytes> instancedData;
			IRender::Resource::QuickDrawCallDescription drawCallDescription;
			TShared<ShaderResource> shaderResource;
			TShared<RenderPolicy> renderPolicy;
			TShared<AnimationComponent> animationComponent;
			IRender::Resource::RenderStateDescription renderStateDescription;
			uint32_t instanceCount;
#ifdef _DEBUG
			String description;
#endif
		};

		class MergedInstanceGroup {
		public:
			MergedInstanceGroup();

			InstanceGroup* groupPrototype;
			std::vector<Bytes> mergedInstanceData;
			uint32_t mergedInstanceCount;
		};

		class TaskData : public TReflected<TaskData, SharedTiny> {
		public:
			TaskData(uint32_t warpCount);
			~TaskData() override;
			void Cleanup(IRender& render);
			void Destroy(IRender& render);
			bool Continue() const { return Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED; }

			TObject<IReflect>& operator () (IReflect& reflect) override;

			class GlobalBufferItem {
			public:
				PassBase::PartialUpdater globalUpdater;
				std::vector<IRender::Resource::DrawCallDescription::BufferRange> buffers;
				std::vector<IRender::Resource*> textures;
			};

			class PolicyData {
			public:
				PolicyData();

				IRender::Queue* portQueue;
				IRender::Resource* instanceBuffer;
				uint32_t instanceOffset;
				Bytes instanceData;
				std::vector<KeyValue<ShaderResource*, GlobalBufferItem> > worldGlobalBufferMap;
				std::vector<KeyValue<IRender::Resource::RenderStateDescription, IRender::Resource*> > renderStateMap;
				std::vector<IRender::Resource*> runtimeResources;
			};

			class_aligned(CPU_CACHELINE_SIZE) WarpData {
			public:
				WarpData();

				typedef std::unordered_map<InstanceKey, InstanceGroup> InstanceGroupMap;
				InstanceGroupMap instanceGroups;
				std::vector<std::pair<RenderPolicy*, LightElement> > lightElements;
				std::vector<std::pair<RenderPolicy*, EnvCubeElement> > envCubeElements;
				std::vector<IDataUpdater*> dataUpdaters;
				BytesCache bytesCache;
				uint32_t entityCount;
				uint32_t visibleEntityCount;
				uint32_t triangleCount;
			};

			std::vector<KeyValue<RenderPolicy*, PolicyData> > renderPolicyMap;
			// std::vector<IDataUpdater*> dataUpdaters;
			WorldGlobalData worldGlobalData;
			BytesCache globalBytesCache;
#if defined(_MSC_VER) && _MSC_VER <= 1200
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup> MergedInstanceGroupMap;
#else
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup, std::hash<InstanceKey>, std::equal_to<InstanceKey>, TCacheAllocator<std::pair<const InstanceKey, MergedInstanceGroup> > > MergedInstanceGroupMap;
#endif
			MergedInstanceGroupMap mergedInstancedGroupMap;
			std::vector<WarpData> warpData;
			std::atomic<uint32_t> pendingCount;
		};
	};

	class CameraComponent : public TAllocatedTiny<CameraComponent, Component>, protected PerspectiveCamera, public SpaceTraversal<CameraComponent, CameraComponentConfig> {
	public:
		enum {
			CAMERACOMPONENT_PERSPECTIVE = COMPONENT_CUSTOM_BEGIN,
			CAMERACOMPONENT_SUBPIXEL_JITTER = COMPONENT_CUSTOM_BEGIN << 1,
			CAMERACOMPONENT_UPDATE_COLLECTING = COMPONENT_CUSTOM_BEGIN << 2,
			CAMERACOMPONENT_UPDATE_COLLECTED = COMPONENT_CUSTOM_BEGIN << 3,
			CAMERACOMPONENT_UPDATE_COMMITTED = COMPONENT_CUSTOM_BEGIN << 4,
			CAMERACOMPONENT_AGILE_RENDERING = COMPONENT_CUSTOM_BEGIN << 5,
			CAMERACOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 6
		};

		friend class SpaceTraversal<CameraComponent, CameraComponentConfig>;

		typedef typename CameraComponentConfig::InstanceKey InstanceKey;
		typedef typename CameraComponentConfig::InstanceGroup InstanceGroup;
		typedef typename CameraComponentConfig::MergedInstanceGroup MergedInstanceGroup;

		CameraComponent(const TShared<RenderFlowComponent>& renderFlowComponent, const String& cameraViewPortName);
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		Tiny::FLAG GetEntityFlagMask() const override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void BindRootEntity(Engine& engine, BridgeComponentModule& bridgeComponentModule, Entity* entity);
		void Refresh(Engine& engine);
		const TShared<BridgeComponent>& GetBridgeComponent() const;
		uint32_t GetCollectedEntityCount() const;
		uint32_t GetCollectedVisibleEntityCount() const;
		uint32_t GetCollectedTriangleCount() const;
		uint32_t GetCollectedDrawCallCount() const;
		float GetViewDistance() const;
		void SetViewDistance(float dist);
		float GetJitterScale() const;
		void SetJitterScale(float scale);
		float GetJitterHistoryRatio() const;
		void SetJitterHistoryRatio(float ratio);
		void SetPerspective(float d, float n, float f, float r);
		void GetPerspective(float& d, float& n, float& f, float& r) const;
		RenderFlowComponent* GetRenderFlowComponent() const;
		TShared<TaskData> GetTaskData();

		// collected cache
		typedef CameraComponentConfig::LightElement LightElement;
		typedef CameraComponentConfig::EnvCubeElement EnvCubeElement;

	protected:
		void DispatchEvent(Event& event, Entity* entity) override;
		void OnTickHost(Engine& engine, Entity* entity);
		void OnTickCameraViewPort(Engine& engine, RenderPort& renderPort, IRender::Queue* queue);
		void UpdateTaskData(Engine& engine, Entity* hostEntity);
		void Instancing(Engine& engine, TaskData& taskData);
		void CommitRenderRequests(Engine& engine, TaskData& taskData, IRender::Queue* queue);

		void CollectLightComponent(Engine& engine, LightComponent* lightComponent, std::vector<std::pair<RenderPolicy*, LightElement> >& lightElements, const MatrixFloat4x4& worldTransform, const TaskData& taskData) const;
		void CollectEnvCubeComponent(EnvCubeComponent* envCubeComponent, std::vector<std::pair<RenderPolicy*, EnvCubeElement> >& envCubeElements, const MatrixFloat4x4& worldMatrix) const;
		void CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData);
		void CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* rootEntity);
		void CompleteCollect(Engine& engine, TaskData& taskData);
		void UpdateRootMatrices(TaskData& taskData, const MatrixFloat4x4& cameraWorldMatrix);
		void UpdateJitterMatrices(CameraComponentConfig::WorldGlobalData& worldGlobalData);

	protected:
		// TaskDatas
		Entity* hostEntity;
		TShared<TaskData> prevTaskData;
		TShared<TaskData> nextTaskData;

		// Runtime Resource
		TShared<RenderFlowComponent> renderFlowComponent;
		TShared<BridgeComponent> bridgeComponent;
		String cameraViewPortName;
		uint32_t collectedEntityCount;
		uint32_t collectedVisibleEntityCount;
		uint32_t collectedTriangleCount;
		uint32_t collectedDrawCallCount;
		float viewDistance;
		float jitterScale;
		float jitterHistoryRatio;
		uint32_t jitterIndex;

		// applied if CAMERACOMPONENT_SMOOTH_TRACK enabled
		struct State {
			QuaternionFloat rotation;
			Float3 translation;
			Float3 scale;
		} targetState, currentState;
	};
}

