// LightComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../../../Core/Template/TCache.h"
#include "../Renderable/RenderableComponent.h"
#include "../Animation/AnimationComponent.h"
#include "../Stream/StreamComponent.h"
#include "../Explorer/SpaceTraversal.h"
#include "../Explorer/CameraCuller.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/Passes/ConstMapPass.h"

namespace PaintsNow {
	struct LightComponentConfig {
		class CaptureData : public FrustrumCuller {
		public:
			Bytes visData;
		};

		class WorldInstanceData : public TReflected<WorldInstanceData, PassBase::PartialData> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override;

			MatrixFloat4x4 worldMatrix;
			Float4 instancedColor; // currently not used.
			Float3Pair boundingBox;
			TShared<AnimationComponent> animationComponent;
		};

		typedef uint32_t InstanceKey;

		class InstanceGroup {
		public:
			InstanceGroup();
			void Cleanup();

			PassBase::PartialUpdater instanceUpdater;
			std::vector<Bytes> instancedData;
			TShared<ShaderResource> shaderResource;
			IRender::Resource::QuickDrawCallDescription drawCallDescription;
			TShared<AnimationComponent> animationComponent;
			uint32_t instanceCount;
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
			TaskData(Engine& engine, uint32_t warpCount, const UShort2& res);
			void Destroy(IRender& render);
			void RenderFrame(Engine& engine);
			bool Continue() const { return Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED; }

			TObject<IReflect>& operator () (IReflect& reflect) override;

			class_aligned(CPU_CACHELINE_SIZE) WarpData {
			public:
				typedef std::unordered_map<InstanceKey, InstanceGroup> InstanceGroupMap;
				InstanceGroupMap instanceGroups;
				BytesCache bytesCache;
			};

			std::vector<WarpData> warpData;
			BytesCache globalBytesCache;
#if defined(_MSC_VER) && _MSC_VER <= 1200
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup> MergedInstanceGroupMap;
#else
			typedef std::unordered_map<InstanceKey, MergedInstanceGroup, std::hash<InstanceKey>, std::equal_to<InstanceKey>, TCacheAllocator<std::pair<const InstanceKey, MergedInstanceGroup> > > MergedInstanceGroupMap;
#endif
			std::atomic<uint32_t> pendingCount;
			TShared<Entity> rootEntity;
			TShared<SharedTiny> shadowGrid;
			IRender::Queue* renderQueue;
			IRender::Resource* stateResource;
			IRender::Resource* renderTargetResource;
		};
	};

	class LightComponent : public TAllocatedTiny<LightComponent, RenderableComponent> {
	public:
		LightComponent();
		friend class SpaceTraversal<LightComponent, LightComponentConfig>;

		typedef typename LightComponentConfig::InstanceKey InstanceKey;
		typedef typename LightComponentConfig::InstanceGroup InstanceGroup;
		typedef typename LightComponentConfig::MergedInstanceGroup MergedInstanceGroup;
		typedef typename LightComponentConfig::TaskData TaskData;

		enum {
			LIGHTCOMPONENT_DIRECTIONAL = RENDERABLECOMPONENT_CUSTOM_BEGIN,
			LIGHTCOMPONENT_CAST_SHADOW = RENDERABLECOMPONENT_CUSTOM_BEGIN << 1,
			LIGHTCOMPONENT_CUSTOM_BEGIN = RENDERABLECOMPONENT_CUSTOM_BEGIN << 2
		};

		TObject<IReflect>& operator () (IReflect& reflect) override;
		FLAG GetEntityFlagMask() const override;
		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;
		void UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) override;
		void Uninitialize(Engine& engine, Entity* entity) override;

		const Float3& GetColor() const;
		void SetColor(const Float3& color);
		float GetAttenuation() const;
		void SetAttenuation(float value);
		const Float3& GetRange() const;
		void SetRange(const Float3& range);

		class ShadowGrid : public TReflected<ShadowGrid, SharedTiny> {
		public:
			TShared<TextureResource> texture;
			MatrixFloat4x4 shadowMatrix;
		};

		class ShadowContext : public TReflected<ShadowContext, SharedTiny> {
		public:
			MatrixFloat4x4 lightTransformMatrix;
			TShared<Entity> rootEntity;
		};

		class ShadowLayer : public TReflected<ShadowLayer, SharedTiny>, public SpaceTraversal<ShadowLayer, LightComponentConfig> {
		public:
			ShadowLayer(Engine& engine);
			TShared<SharedTiny> StreamLoadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context);
			void StreamRefreshHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context);
			TShared<SharedTiny> StreamUnloadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context);
			void CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData);
			void CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* rootEntity);
			void CompleteCollect(Engine& engine, TaskData& taskData);
			void Initialize(Engine& engine, const TShared<StreamComponent>& streamComponent, const UShort2& res, float size, float scale);
			void Uninitialize(Engine& engine);

			void UpdateShadow(Engine& engine, const MatrixFloat4x4& cameraTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity);
			const TShared<ShadowGrid>& GetCurrent() const;

		protected:
			TShared<StreamComponent> streamComponent;
			TShared<ShaderResourceImpl<ConstMapPass> > pipeline;
			TShared<TaskData> currentTask;
			TShared<ShadowGrid> currentGrid; // last known good one
			float gridSize;
			float scale;
			UShort2 resolution;
		};

		const std::vector<TShared<ShadowLayer> >& UpdateShadow(Engine& engine, const MatrixFloat4x4& viewTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity);
		void BindShadowStream(Engine& engine, uint32_t layer, const TShared<StreamComponent>& streamComponent, const UShort2& res, float gridSize, float scale);

	protected:
		Float3 color;
		float attenuation;
		Float3 range;
		// float spotAngle;
		// float temperature;
		std::vector<TShared<ShadowLayer> > shadowLayers;
	};
}
