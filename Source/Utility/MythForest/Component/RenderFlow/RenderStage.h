// RenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "RenderPort.h"
#include "../../Engine.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	class RenderFlowComponent;
	class RenderStage : public TReflected<RenderStage, GraphNode<SharedTiny, RenderPort> > {
	public:
		RenderStage(uint32_t colorAttachmentCount = 1);
		friend class RenderFlowComponent;

		enum {
			RENDERSTAGE_WEAK_LINKAGE = TINY_CUSTOM_BEGIN,
			RENDERSTAGE_ADAPT_MAIN_RESOLUTION = TINY_CUSTOM_BEGIN << 1,
			RENDERSTAGE_RENDERTARGET_MODIFIED = TINY_CUSTOM_BEGIN << 2,
			RENDERSTAGE_COMPUTE_PASS = TINY_CUSTOM_BEGIN << 3,
			RENDERSTAGE_ENABLED = TINY_CUSTOM_BEGIN << 4,
			RENDERSTAGE_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN << 5
		};

		virtual void Initialize(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Uninitialize(Engine& engine, IRender::Queue* resourceQueue);
		virtual void PreInitialize(Engine& engine, IRender::Queue* resourceQueue);

		virtual void OnFrameUpdate(Engine& engine, IRender::Queue* resourceQueue);
		virtual void OnFrameResolutionUpdate(Engine& engine, IRender::Queue* resourceQueue, UShort2 res);
		virtual void OnFrameTick(Engine& engine, IRender::Queue* resourceQueue);
		virtual uint32_t OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& repeatQueues, IRender::Queue* instantMainQueue);
		virtual uint32_t OnSetupRenderTarget(Engine& engine, IRender::Queue* queue);

		uint16_t GetFrameBarrierIndex() const;
		void SetFrameBarrierIndex(uint16_t index);
		IRender::Resource* GetRenderTargetResource() const;
		const IRender::Resource::RenderTargetDescription& GetRenderTargetDescription() const;
		friend class FrameBarrierRenderStage;

		IRender::Resource::RenderStateDescription renderStateDescription;
		IRender::Resource::RenderTargetDescription renderTargetDescription;
		TShared<MaterialResource> overrideMaterial;

	protected:
		void RefreshOverrideMaterial(Engine& engine, IRender::Queue* queue, ShaderResource* shaderInstance, TShared<MaterialResource>& materialInstance);

	protected:
		IRender::Resource* renderState;
		IRender::Resource* renderTarget;
		Char2 resolutionShift;
		uint16_t frameBarrierIndex;
		std::vector<IRender::Resource*> newResources;
	};

	template <class T>
	class GeneralRenderStage : public TReflected<GeneralRenderStage<T>, RenderStage> {
	public:
		// gcc do not support referencing base type in template class. manunaly specified here.
		typedef TReflected<GeneralRenderStage<T>, RenderStage> BaseClass;
		GeneralRenderStage(uint32_t colorAttachmentCount = 1) : BaseClass(colorAttachmentCount) {}
		void PreInitialize(Engine& engine, IRender::Queue* queue) override {
			// create specified shader resource (if not exists)
			String path = ShaderResource::GetShaderPathPrefix() + UniqueType<T>::Get()->GetBriefName();
			sharedShader = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);
			shaderInstance.Reset(static_cast<ShaderResourceImpl<T>*>(sharedShader->Clone()));

			// Process compute shader stage.
			if (GetPass().ExportShaderStageMask() & (1 << IRender::Resource::ShaderDescription::COMPUTE)) {
				BaseClass::Flag().fetch_or(BaseClass::RENDERSTAGE_COMPUTE_PASS, std::memory_order_relaxed);
			}

			BaseClass::PreInitialize(engine, queue);
		}

	protected:
		inline IRender::Resource* GetShaderResource() const {
			if (overrideMaterialInstance) {
				assert(overrideMaterialInstance->originalShaderResource);
				return overrideMaterialInstance->originalShaderResource->GetShaderResource();
			} else {
				IRender::Resource* resource = shaderInstance->GetShaderResource();
				return resource == nullptr ? sharedShader->GetShaderResource() : resource;
			}
		}

		inline T& GetPass() {
			return static_cast<T&>(shaderInstance->GetPass());
		}

		PassBase::Updater& GetPassUpdater() {
			return shaderInstance->GetPassUpdater();
		}

		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override {
			BaseClass::RefreshOverrideMaterial(engine, queue, shaderInstance(), overrideMaterialInstance);
			BaseClass::OnFrameUpdate(engine, queue);
		}

	protected:
		TShared<ShaderResourceImpl<T> > shaderInstance;
		TShared<ShaderResource> sharedShader;
		TShared<MaterialResource> overrideMaterialInstance;
	};

	template <class T>
	class GeneralRenderStageDraw : public TReflected<GeneralRenderStageDraw<T>, GeneralRenderStage<T> > {
	public:
		// gcc do not support referencing base type in template class. manunaly specified here.
		typedef TReflected<GeneralRenderStageDraw<T>, GeneralRenderStage<T> > BaseClass;
		GeneralRenderStageDraw(uint32_t colorAttachmentCount = 1) : BaseClass(colorAttachmentCount), drawCallResource(nullptr) {
			drawCallDescription.shaderResource = nullptr;
		}

		void PreInitialize(Engine& engine, IRender::Queue* queue) override {
			// create specified shader resource (if not exists)
			if (!(BaseClass::Flag().load(std::memory_order_relaxed) & BaseClass::RENDERSTAGE_COMPUTE_PASS)) {
				// prepare mesh
				if (!meshResource) {
					const String path = "[Runtime]/MeshResource/StandardQuad";
					meshResource = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);
					assert(meshResource);
				}
			}

			BaseClass::PreInitialize(engine, queue);
		}

		void Uninitialize(Engine& engine, IRender::Queue* queue) override {
			BaseClass::Uninitialize(engine, queue);
			if (drawCallResource != nullptr) {
				IRender& render = engine.interfaces.render;
				render.DeleteResource(queue, drawCallResource);
			}
		}

		// Helper functions
		void OnFrameUpdate(Engine& engine, IRender::Queue* queue) override {
			BaseClass::OnFrameUpdate(engine, queue);

			IRender& render = engine.interfaces.render;
			PassBase::Updater& updater = BaseClass::GetPassUpdater();
			std::vector<Bytes> bufferData;
			updater.Capture(drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
			// first time?
			if (drawCallResource == nullptr) {
				assert(BaseClass::newResources.empty());
				updater.Update(render, queue, drawCallDescription, BaseClass::newResources, bufferData,
					(1 << IRender::Resource::BufferDescription::VERTEX) |
					(1 << IRender::Resource::BufferDescription::UNIFORM) |
					(1 << IRender::Resource::BufferDescription::STORAGE) |
					(1 << IRender::Resource::BufferDescription::INSTANCED));
				drawCallDescription.shaderResource = BaseClass::GetShaderResource();
				drawCallResource = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
				IRender::Resource::DrawCallDescription::BufferRange& bufferRange = drawCallDescription.indexBufferResource;
				if (!(BaseClass::Flag().load(std::memory_order_relaxed) & BaseClass::RENDERSTAGE_COMPUTE_PASS)) {
					bufferRange.buffer = meshResource->bufferCollection.indexBuffer;
				} else {
					bufferRange.buffer = nullptr;
				}

				bufferRange.offset = 0;
				bufferRange.length = 0;
				bufferRange.component = 0;
				bufferRange.type = 0;
			} else {
				// recapture all data (uniforms by default)
				size_t count = BaseClass::newResources.size();
				updater.Update(render, queue, drawCallDescription, BaseClass::newResources, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
				assert(count == BaseClass::newResources.size()); // must not add new resource(s)
			}

			IRender::Resource::DrawCallDescription& copy = *static_cast<IRender::Resource::DrawCallDescription*>(render.MapResource(queue, drawCallResource, 0));
			copy = drawCallDescription;
			render.UnmapResource(queue, drawCallResource, IRender::MAP_DATA_EXCHANGE);
		}

		uint32_t OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& repeatQueues, IRender::Queue* instantMainQueue) override {
			uint32_t count = BaseClass::OnFrameCommit(engine, repeatQueues, instantMainQueue);
			IRender& render = engine.interfaces.render;
			render.ExecuteResource(instantMainQueue, drawCallResource);
			return count + 1;
		}

	protected:
		TShared<MeshResource> meshResource;
		IRender::Resource::DrawCallDescription drawCallDescription;
		IRender::Resource* drawCallResource;
	};
}

