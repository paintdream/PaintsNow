#include "RenderPortRenderTarget.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortRenderTargetLoad

RenderPortRenderTargetLoad::RenderPortRenderTargetLoad(IRender::Resource::RenderTargetDescription::Storage& storage, bool write) : bindingStorage(storage) {
	if (write) {
		Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_relaxed);
	}
}

TObject<IReflect>& RenderPortRenderTargetLoad::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {}

	return *this;
}

void RenderPortRenderTargetLoad::Initialize(Engine& engine, IRender::Queue* mainQueue) {
	OnFramePreTick(engine, mainQueue);
}

void RenderPortRenderTargetLoad::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {}

void RenderPortRenderTargetLoad::OnFramePreTick(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFramePreTick(engine, queue);
	if (GetLinks().empty()) return;

	RenderPort* port = static_cast<RenderPort*>(GetLinks().back().port);
	RenderPortRenderTargetStore* target = port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
	if (target != nullptr) {
		RenderStage* renderStage = static_cast<RenderStage*>(port->GetNode());
		RenderStage* hostRenderStage = static_cast<RenderStage*>(GetNode());

		if (bindingStorage.resource != target->bindingStorage.resource || bindingStorage.mipLevel != target->bindingStorage.mipLevel) {
			bindingStorage.resource = target->bindingStorage.resource;
			bindingStorage.mipLevel = target->bindingStorage.mipLevel;

			// Link to correspond RenderTargetStore
			const std::vector<RenderStage::PortInfo>& ports = hostRenderStage->GetPorts();
			for (size_t i = 0; i < ports.size(); i++) {
				const RenderStage::PortInfo& port = ports[i];
				RenderPortRenderTargetStore* t = port.port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
				if (t != nullptr && &t->bindingStorage == &bindingStorage) {
					t->attachedTexture = target->attachedTexture;
					break;
				}
			}

			Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
		}
	}
}

RenderPortRenderTargetStore* RenderPortRenderTargetLoad::QueryStore() const {
	const std::vector<RenderStage::PortInfo>& portInfos = static_cast<RenderStage*>(GetNode())->GetPorts();
	for (size_t i = 0; i < portInfos.size(); i++) {
		RenderPortRenderTargetStore* rt = portInfos[i].port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
		if (rt != nullptr && &rt->bindingStorage == &bindingStorage)
			return rt;
	}

	return nullptr;
}

uint32_t RenderPortRenderTargetLoad::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) {
	if (GetLinks().empty())
		return 0;

	IRender& render = engine.interfaces.render;
	RenderPort* port = static_cast<RenderPort*>(GetLinks().back().port);
	RenderPortRenderTargetStore* target = port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
	if (target != nullptr) {
		TextureResource* texture = target->attachedTexture();
		if (texture != nullptr) {
			IRender::Barrier barrier; // image barrier
			IRender::Resource::RenderTargetDescription::Storage& depthStorage = static_cast<RenderStage*>(target->GetNode())->renderTargetDescription.depthStorage;
			IRender::Resource::RenderTargetDescription::Storage& stencilStorage = static_cast<RenderStage*>(target->GetNode())->renderTargetDescription.stencilStorage;

			barrier.resource = texture->GetRenderResource();

			// is depth stencil?
			if (&target->bindingStorage == &depthStorage || &target->bindingStorage == &stencilStorage) {
				barrier.srcAccessMask = IRender::ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = IRender::ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				barrier.aspectMask = (IRender::AspectFlagBits)(IRender::ASPECT_DEPTH_BIT | IRender::ASPECT_STENCIL_BIT);
				barrier.oldLayout = IRender::LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				barrier.newLayout = IRender::LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				barrier.srcStageMask = IRender::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				barrier.dstStageMask = IRender::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			} else {
				barrier.srcAccessMask = IRender::ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = IRender::ACCESS_COLOR_ATTACHMENT_READ_BIT;
				barrier.aspectMask = IRender::ASPECT_COLOR_BIT;
				barrier.oldLayout = IRender::LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				barrier.newLayout = IRender::LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				barrier.srcStageMask = IRender::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				barrier.dstStageMask = IRender::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}

			barrier.baseMipLevel = 0;
			barrier.levelCount = 1;
			barrier.baseArrayLayer = 0;
			barrier.layerCount = 1;
			barrier.dependencyMask = IRender::DEPENDENCY_BY_REGION_BIT;

			render.SetupBarrier(queue, &barrier);
			return 1;
		}
	}

	return 0;
}

// RenderPortRenderTargetStore

RenderPortRenderTargetStore::RenderPortRenderTargetStore(IRender::Resource::RenderTargetDescription::Storage& storage) : bindingStorage(storage) {
	renderTargetDescription.state.attachment = true;
	renderTargetDescription.state.immutable = false;
	renderTargetDescription.dimension = UShort3(1, 1, 0);
}

TObject<IReflect>& RenderPortRenderTargetStore::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderTargetDescription);
	}

	return *this;
}

void RenderPortRenderTargetStore::Initialize(Engine& engine, IRender::Queue* mainQueue) {
	OnFramePreTick(engine, mainQueue);
}

void RenderPortRenderTargetStore::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {}

void RenderPortRenderTargetStore::OnFramePreTick(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFramePreTick(engine, queue);

	if (attachedTexture) {
		if (bindingStorage.resource != attachedTexture->GetRenderResource()) {
			bindingStorage.resource = attachedTexture->GetRenderResource();
			GetNode()->Flag().fetch_or(TINY_MODIFIED | RenderStage::RENDERSTAGE_RENDERTARGET_MODIFIED, std::memory_order_relaxed);
		}
	}
}

RenderPortRenderTargetLoad* RenderPortRenderTargetStore::QueryLoad() const {
	const std::vector<RenderStage::PortInfo>& portInfos = static_cast<RenderStage*>(GetNode())->GetPorts();
	for (size_t i = 0; i < portInfos.size(); i++) {
		RenderPortRenderTargetLoad* rt = portInfos[i].port->QueryInterface(UniqueType<RenderPortRenderTargetLoad>());
		if (rt != nullptr && &rt->bindingStorage == &bindingStorage)
			return rt;
	}

	return nullptr;
}

uint32_t RenderPortRenderTargetStore::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) {
	// Am I the first pass to render a texture?
	if (QueryLoad() == nullptr) {
		IRender& render = engine.interfaces.render;
		TextureResource* texture = attachedTexture();
		IRender::Barrier barrier; // image barrier
		IRender::Resource::RenderTargetDescription::Storage& depthStorage = static_cast<RenderStage*>(GetNode())->renderTargetDescription.depthStorage;
		IRender::Resource::RenderTargetDescription::Storage& stencilStorage = static_cast<RenderStage*>(GetNode())->renderTargetDescription.stencilStorage;

		barrier.resource = texture->GetRenderResource();

		// is depth stencil?
		if (&bindingStorage == &depthStorage || &bindingStorage == &stencilStorage) {
			barrier.srcAccessMask = IRender::ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = IRender::ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			barrier.aspectMask = (IRender::AspectFlagBits)(IRender::ASPECT_DEPTH_BIT | IRender::ASPECT_STENCIL_BIT);
			barrier.oldLayout = IRender::LAYOUT_UNDEFINED;
			barrier.newLayout = IRender::LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			barrier.srcStageMask = IRender::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			barrier.dstStageMask = IRender::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		} else {
			barrier.srcAccessMask = IRender::ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = IRender::ACCESS_COLOR_ATTACHMENT_READ_BIT;
			barrier.aspectMask = IRender::ASPECT_COLOR_BIT;
			barrier.oldLayout = IRender::LAYOUT_UNDEFINED;
			barrier.newLayout = IRender::LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcStageMask = IRender::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			barrier.dstStageMask = IRender::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		barrier.baseMipLevel = 0;
		barrier.levelCount = 1;
		barrier.baseArrayLayer = 0;
		barrier.layerCount = 1;
		barrier.dependencyMask = IRender::DEPENDENCY_BY_REGION_BIT;

		render.SetupBarrier(queue, &barrier);
		return 1;
	}

	return 0;
}
