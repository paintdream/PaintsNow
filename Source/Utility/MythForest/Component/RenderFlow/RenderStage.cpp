#include "RenderStage.h"
#include "RenderPort/RenderPortRenderTarget.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"

using namespace PaintsNow;

RenderStage::RenderStage(uint32_t colorAttachmentCount) : renderState(nullptr), renderTarget(nullptr), resolutionShift(0, 0), frameBarrierIndex(0) {
	Flag().fetch_or(RENDERSTAGE_ADAPT_MAIN_RESOLUTION, std::memory_order_relaxed);

	// Initialize state
	IRender::Resource::RenderStateDescription& s = renderStateDescription;
	s.stencilReplacePass = 0;
	s.cull = 1;
	s.fill = 1;
	s.blend = 0;
	s.colorWrite = 1;
	s.depthTest = 0;
	s.depthWrite = 0;
	s.stencilTest = 0;
	s.stencilWrite = 0;
	s.stencilValue = 0;
	s.stencilMask = 0;

	IRender::Resource::RenderTargetDescription& t = renderTargetDescription;

	t.colorStorages.resize(colorAttachmentCount);
	for (size_t i = 0; i < t.colorStorages.size(); i++) {
		IRender::Resource::RenderTargetDescription::Storage& s = t.colorStorages[i];
		s.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
		s.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	}

	t.depthStorage.loadOp = t.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
	t.depthStorage.storeOp = t.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	t.dimension = UShort3(0, 0, 0);
}

uint16_t RenderStage::GetFrameBarrierIndex() const {
	return frameBarrierIndex;
}

void RenderStage::SetFrameBarrierIndex(uint16_t index) {
	frameBarrierIndex = index;
}

void RenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	if (!(Flag().load(std::memory_order_relaxed) & RENDERSTAGE_COMPUTE_PASS)) {
		IRender& render = engine.interfaces.render;
		IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();

		assert(renderState == nullptr);
		assert(renderTarget == nullptr);
		assert(!renderTargetDescription.colorStorages.empty());

		renderState = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERSTATE);
		IRender::Resource::RenderStateDescription& rs = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(queue, renderState, 0));
		rs = renderStateDescription;
		render.UnmapResource(queue, renderState, IRender::MAP_DATA_EXCHANGE);
		renderTarget = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERTARGET);
		render.SetResourceNote(renderTarget, GetUnique()->GetBriefName());
	}
}

void RenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	if ((Flag().fetch_and(~RENDERSTAGE_RENDERTARGET_MODIFIED, std::memory_order_acquire) & RENDERSTAGE_RENDERTARGET_MODIFIED)) {
		IRender& render = engine.interfaces.render;
		IRender::Resource::RenderTargetDescription& desc = *static_cast<IRender::Resource::RenderTargetDescription*>(render.MapResource(queue, renderTarget, 0));
		desc = renderTargetDescription;

		// optimize for Don't Care (DISCARD)
		const std::vector<PortInfo>& ports = GetPorts();
		for (size_t i = 0; i < ports.size(); i++) {
			RenderPortRenderTargetStore* store = ports[i].port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
			// Not referenced by any other nodes
			if (store != nullptr && store->GetLinks().empty() && store->attachedTexture) {
				// can be safety discarded?
				if (store->attachedTexture->description.frameBarrierIndex < frameBarrierIndex) {
					if (&renderTargetDescription.depthStorage == &store->bindingStorage) {
						desc.depthStorage.storeOp = desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
					} else if (&renderTargetDescription.stencilStorage == &store->bindingStorage) {
						desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
					} else {
						size_t index = &store->bindingStorage - &renderTargetDescription.colorStorages[0];
						desc.colorStorages[index].storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
					}
				}
			}

			RenderPortRenderTargetLoad* load = ports[i].port->QueryInterface(UniqueType<RenderPortRenderTargetLoad>());
			if (load != nullptr && !load->GetLinks().empty()) {
				assert(load->GetLinks().size() == 1);
				RenderPortRenderTargetStore* upstream = static_cast<RenderPortRenderTargetStore*>(load->GetLinks().back().port);
				assert(upstream != nullptr);
				if (upstream->bindingStorage.storeOp == IRender::Resource::RenderTargetDescription::DISCARD) {
					assert(load->bindingStorage.loadOp != IRender::Resource::RenderTargetDescription::DEFAULT);
				}
			}
		}

		assert(desc.depthStorage.loadOp == desc.stencilStorage.loadOp);
		assert(desc.depthStorage.storeOp == desc.stencilStorage.storeOp);

		render.UnmapResource(queue, renderTarget, IRender::MAP_DATA_EXCHANGE);
	}

	const std::vector<PortInfo>& ports = GetPorts();
	for (size_t i = 0; i < ports.size(); i++) {
		ports[i].port->OnFrameUpdate(engine, queue);
	}
}

void RenderStage::Initialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->Initialize(engine, queue);
	}

	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
	OnFrameTick(engine, queue);
}

void RenderStage::Uninitialize(Engine& engine, IRender::Queue* queue) {
	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release);

	IRender& render = engine.interfaces.render;
	for (size_t k = 0; k < nodePorts.size(); k++) {
		nodePorts[k].port->Uninitialize(engine, queue);
	}

	for (size_t i = 0; i < newResources.size(); i++) {
		render.DeleteResource(queue, newResources[i]);
	}

	newResources.clear();

	if (renderState != nullptr) {
		render.DeleteResource(queue, renderState);
	}

	if (renderTarget != nullptr) {
		render.DeleteResource(queue, renderTarget);
	}
}

void RenderStage::OnFrameTick(Engine& engine, IRender::Queue* queue) {
	Tiny::FLAG flag = 0;
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->OnFramePreTick(engine, queue);
		flag |= nodePorts[i].port->Flag().fetch_and(~TINY_MODIFIED, std::memory_order_relaxed);
	}

	flag |= Flag().fetch_and(~TINY_MODIFIED, std::memory_order_relaxed);

	if (flag & TINY_MODIFIED) {
		OnFrameUpdate(engine, queue);
	}

	for (size_t k = 0; k < nodePorts.size(); k++) {
		nodePorts[k].port->OnFramePostTick(engine, queue);
	}
}

IRender::Resource* RenderStage::GetRenderTargetResource() const {
	return renderTarget;
}

const IRender::Resource::RenderTargetDescription& RenderStage::GetRenderTargetDescription() const {
	return renderTargetDescription;
}

uint32_t RenderStage::OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& repeatQueues, IRender::Queue* instantMainQueue) {
	Tiny::FLAG flag = Flag().load(std::memory_order_relaxed);
	assert(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED);
	size_t countRepeatQueues = repeatQueues.size();
	IRender& render = engine.interfaces.render;

	uint32_t mainCommandCount = OnSetupRenderTarget(engine, instantMainQueue);

	for (size_t i = 0; i < nodePorts.size(); i++) {
		mainCommandCount += nodePorts[i].port->OnFrameCommit(engine, repeatQueues, instantMainQueue);
	}

	return mainCommandCount;
}

uint32_t RenderStage::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	uint32_t mainCommandCount = 2;
	for (size_t i = 0; i < nodePorts.size(); i++) {
		mainCommandCount += nodePorts[i].port->OnSetupRenderTarget(engine, queue);
	}

	render.ExecuteResource(queue, renderState);
	render.ExecuteResource(queue, renderTarget);

	return mainCommandCount;
}

void RenderStage::OnFrameResolutionUpdate(Engine& engine, IRender::Queue* resourceQueue, UShort2 res) {
	if (!(Flag().load(std::memory_order_acquire) & RENDERSTAGE_ADAPT_MAIN_RESOLUTION)) return;

	if (!(flag & RENDERSTAGE_COMPUTE_PASS)) {
		// By default, create render buffer with resolution provided
		// For some stages(e.g. cascaded bloom generator), we must override this function to adapt the new value
		// by now we have no color-free render buffers
		uint16_t width = res.x(), height = res.y();
		IRender& render = engine.interfaces.render;
		assert(width != 0 && height != 0);
		width = verify_cast<uint16_t>(resolutionShift.x() > 0 ? Math::Max(width >> resolutionShift.x(), 2) : width << resolutionShift.x());
		height = verify_cast<uint16_t>(resolutionShift.y() > 0 ? Math::Max(height >> resolutionShift.y(), 2) : height << resolutionShift.y());
		renderTargetDescription.dimension = UShort3(width, height, 1);

		const std::vector<PortInfo>& portInfos = GetPorts();
		for (size_t i = 0; i < portInfos.size(); i++) {
			RenderPortRenderTargetStore* rt = portInfos[i].port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
			if (rt != nullptr) {
				rt->renderTargetDescription.dimension.x() = width;
				rt->renderTargetDescription.dimension.y() = height;

				if (rt->attachedTexture) {
					rt->attachedTexture->description.dimension = rt->renderTargetDescription.dimension;
					IRender::Resource* texture = rt->attachedTexture->GetRenderResource();
					IRender::Resource::TextureDescription& desc = *static_cast<IRender::Resource::TextureDescription*>(render.MapResource(resourceQueue, texture, 0));
					desc = std::move(rt->attachedTexture->description);
					render.UnmapResource(resourceQueue, rt->attachedTexture->GetRenderResource(), IRender::MAP_DATA_EXCHANGE);
				}
			}
		}

		Flag().fetch_or(TINY_MODIFIED | RENDERSTAGE_RENDERTARGET_MODIFIED, std::memory_order_relaxed);
	}
}

void RenderStage::RefreshOverrideMaterial(Engine& engine, IRender::Queue* queue, ShaderResource* shaderInstance, TShared<MaterialResource>& overrideMaterialInstance) {
	if (overrideMaterial) {
		// new shader?
		if (overrideMaterial->originalShaderResource) {
			// create new material instance
			if (!overrideMaterialInstance) {
				overrideMaterialInstance = engine.snowyStream.CreateReflectedResource(UniqueType<MaterialResource>(), "", false, ResourceBase::RESOURCE_VIRTUAL);
				overrideMaterialInstance->originalShaderResource = static_cast<ShaderResource*>(overrideMaterial->originalShaderResource->Clone());
				overrideMaterialInstance->textureResources = overrideMaterial->textureResources;
			}

			// refresh parameter instantly
			overrideMaterialInstance->materialParams.variables.clear();
			overrideMaterialInstance->Import(shaderInstance);
			overrideMaterialInstance->MergeParameters(overrideMaterial->materialParams.variables);
		} else if (overrideMaterial->Flag().load(std::memory_order_acquire) & Tiny::TINY_MODIFIED) {
			if (overrideMaterial->Flag().fetch_and(~Tiny::TINY_MODIFIED) & Tiny::TINY_MODIFIED) {
				// flush material
				overrideMaterial->Export(engine.interfaces.render, queue, shaderInstance);
			}
		}

		// refresh render state
		static_assert(sizeof(overrideMaterial->materialParams.state) == sizeof(uint32_t), "Please use larger integer type here!");
		static_assert(sizeof(overrideMaterial->materialParams.stateMask) == sizeof(uint32_t), "Please use larger integer type here!");
		static_assert(sizeof(renderStateDescription) == sizeof(uint32_t), "Please use larger integer type here!");

		uint32_t renderStateMask = *reinterpret_cast<uint32_t*>(&overrideMaterial->materialParams.state);
		uint32_t renderStateTemplate = *reinterpret_cast<uint32_t*>(&overrideMaterial->materialParams.state);
		uint32_t& renderStateTarget = *reinterpret_cast<uint32_t*>(&renderStateDescription);
		uint32_t newTarget = (renderStateTarget & ~renderStateMask) | (renderStateTemplate | renderStateMask);

		if (renderStateTarget != newTarget) {
			renderStateTarget = newTarget;
			IRender& render = engine.interfaces.render;
			IRender::Resource::RenderStateDescription& desc = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(queue, renderState, 0));
			desc = renderStateDescription;
			render.UnmapResource(queue, renderState, IRender::MAP_DATA_EXCHANGE);
		}
	}
}
