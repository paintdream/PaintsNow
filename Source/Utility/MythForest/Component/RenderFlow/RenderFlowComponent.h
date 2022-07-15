// StandardRenderFlow.h
// PaintDream (paintdream@paintdream.com)
// 2018-8-12
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/System/Graph.h"
#include "RenderStage.h"

namespace PaintsNow {
	class RenderFlowComponent : public TAllocatedTiny<RenderFlowComponent, Component>, protected Graph<RenderStage> {
	public:
		RenderFlowComponent();
		~RenderFlowComponent() override;

		enum {
			RENDERFLOWCOMPONENT_SYNC_DEVICE_RESOLUTION = COMPONENT_CUSTOM_BEGIN,
			RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED = COMPONENT_CUSTOM_BEGIN << 1,
			RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING = COMPONENT_CUSTOM_BEGIN << 2,
			RENDERFLOWCOMPONENT_RENDERING = COMPONENT_CUSTOM_BEGIN << 3,
			RENDERFLOWCOMPONENT_RESOURCE_PREPARED = COMPONENT_CUSTOM_BEGIN << 4,
			RENDERFLOWCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 5
		};

		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void DispatchEvent(Event& event, Entity* entity) override;
		Tiny::FLAG GetEntityFlagMask() const override;

		void AddNode(RenderStage* renderStage);
		void RemoveNode(RenderStage* renderStage);
		// set 0 for screen size
		UShort2 GetMainResolution() const;
		void OnFrameResolutionUpdate(const UShort2 res);

		RenderStage::Port* BeginPort(const String& symbol);
		void EndPort(RenderStage::Port* port);
		bool ExportSymbol(const String& symbol, RenderStage* renderStage, const String& port);
		void Compile(Engine& engine);

	protected:
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void OnFrameResolutionUpdate(Engine& engine);
		void MarkupRenderStages();
		void ResolveSamplessAttachments();
		void SetupTextures(Engine& engine);
		void Render(Engine& engine);
		void RenderSyncTick(Engine& engine);

		UShort2 mainResolution;
		std::map<String, std::pair<RenderStage*, String> > symbolMap;
		std::vector<RenderStage*> cachedRenderStages;
		IRender::Queue* mainQueue;
		IThread::Lock* frameSyncLock;
		IThread::Event* frameSyncEvent;
		IRender::Resource* eventResourcePrepared;
	};
}

