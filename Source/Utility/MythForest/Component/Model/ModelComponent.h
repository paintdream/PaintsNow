// ModelComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../../Component/Batch/BatchComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	class ModelComponent : public TAllocatedTiny<ModelComponent, RenderableComponent> {
	public:
		enum {
			MODELCOMPONENT_HAS_ANIMATION = RENDERABLECOMPONENT_CUSTOM_BEGIN,
			MODELCOMPONENT_CUSTOM_BEGIN = RENDERABLECOMPONENT_CUSTOM_BEGIN << 1
		};

		// delayed loader
		ModelComponent(const TShared<MeshResource>& meshResource, const TShared<BatchComponent>& batch);

		String GetDescription() const override;
		void UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;

		void SetMaterial(uint16_t meshGroupIndex, uint16_t policy, const TShared<MaterialResource>& materialResource);
		void ClearMaterials();
		void UpdateMaterials();

		uint32_t CreateOverrider(const TShared<ShaderResource>& shaderResourceTemplate);
		size_t ReportGraphicMemoryUsage() const override;

		class MaterialEntry {
		public:
			bool operator < (const MaterialEntry& rhs) const {
				return key < rhs.key;
			}

			union {
				struct {
					uint16_t priority;
					uint16_t meshGroupIndex;
				};

				uint32_t key;
			};

			TShared<MaterialResource> material;
		};

		const std::vector<MaterialEntry>& GetMaterials() const;

	protected:
		class CollapseData {
		public:
			String meshResourceLocation;
			std::vector<String> materialResourceLocations;
		};
		
		class OutputRenderDataEx : public IDrawCallProvider::OutputRenderDataBase {
		public:
			void UpdateHashValue(uint64_t materialHashValue);

			IRender::Resource::DrawCallDescription drawCallDescription;
		};

		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;
		virtual void GenerateDrawCalls(std::vector<OutputRenderDataEx>& drawCallTemplates, std::vector<MaterialEntry>& materialResources);
		static void GenerateDrawCall(OutputRenderDataEx& renderData, ShaderResource* shaderResource, std::vector<IRender::Resource*>& meshBuffers, const IAsset::MeshGroup& slice, const MeshResource::BufferCollection& bufferCollection, uint32_t deviceElementSize, uint16_t priority, uint16_t index);

		void Collapse(Engine& engine);
		void Expand(Engine& engine);

	protected:
		std::vector<MaterialEntry> materialResources;
		std::vector<OutputRenderDataEx> drawCallTemplates;
		std::vector<TShared<ShaderResource> > shaderOverriders;
		TShared<MeshResource> meshResource;
		TShared<BatchComponent> batchComponent;
		CollapseData collapseData;
		uint32_t hostCount;
	};
}
