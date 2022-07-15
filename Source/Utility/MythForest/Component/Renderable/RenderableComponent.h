// RenderableComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../../../Core/Template/TCache.h"
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/RenderResourceBase.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/SkeletonResource.h"
#include "RenderPolicy.h"

namespace PaintsNow {
	typedef TCacheAllocator<Bytes> BytesAllocator;

	class IDrawCallProvider {
	public:
		class InputRenderData {
		public:
			InputRenderData(float ref = 0.0f, ShaderResource* res = nullptr, const UShort2 resolution = UShort2(0, 0)) : overrideShaderTemplate(res), viewResolution(resolution), viewReference(ref) {}

			ShaderResource* overrideShaderTemplate;
			UShort2 viewResolution;
			float viewReference;
		};

		class OutputRenderDataBase {
		public:
			IRender::Resource::RenderStateDescription renderStateDescription;
			IDataUpdater* dataUpdater;
			TShared<ShaderResource> shaderResource;
			TShared<SharedTiny> host;
			uint16_t priority;
			uint16_t groupIndex;
			uint32_t hashValue;
		};

		typedef TCacheAllocator<MatrixFloat4x4> LocalMatrixAllocator;
		typedef TCacheAllocator<std::pair<uint32_t, Bytes> > LocalInstanceAllocator;

		class OutputRenderData : public OutputRenderDataBase {
		public:
			OutputRenderData(BytesCache& bytesCache) : localTransforms(&bytesCache), localInstancedData(&bytesCache) {}
			IRender::Resource::QuickDrawCallDescription drawCallDescription;
			std::vector<MatrixFloat4x4, LocalMatrixAllocator> localTransforms;
			std::vector<std::pair<uint32_t, Bytes>, LocalInstanceAllocator> localInstancedData;
		};

		enum CollectOption {
			COLLECT_DEFAULT = 0,
			COLLECT_AGILE_RENDERING = 1,
		};

		typedef TCacheAllocator<OutputRenderData> DrawCallAllocator;
		virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) = 0;
	};

	class RenderableComponent : public TAllocatedTiny<RenderableComponent, Component>, public IDrawCallProvider {
	public:
		enum {
			RENDERABLECOMPONENT_CAMERAVIEW = COMPONENT_CUSTOM_BEGIN,
			RENDERABLECOMPONENT_INVISIBLE = COMPONENT_CUSTOM_BEGIN << 1,
			RENDERABLECOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 2
		};

		RenderableComponent();

		Tiny::FLAG GetEntityFlagMask() const override;
		virtual size_t ReportGraphicMemoryUsage() const;
		const std::vector<TShared<RenderPolicy> >& GetRenderPolicies() const;
		void AddRenderPolicy(const TShared<RenderPolicy>& renderPolicy);
		void RemoveRenderPolicy(const TShared<RenderPolicy>& renderPolicy);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void SetVisible(bool visible);
		bool GetVisible() const;

	private:
		std::vector<TShared<RenderPolicy> > renderPolicies;
	};
}
