#include "RenderPortTextureInput.h"
#include "RenderPortRenderTarget.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortTextureInput

RenderPortTextureInput::RenderPortTextureInput() {}

TObject<IReflect>& RenderPortTextureInput::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(textureResource);
	}

	return *this;
}

void RenderPortTextureInput::Initialize(Engine& engine, IRender::Queue* mainQueue) {}
void RenderPortTextureInput::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {}

void RenderPortTextureInput::OnFramePreTick(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFramePreTick(engine, queue);
	if (GetLinks().empty()) return;

	RenderPortRenderTargetStore* rt = GetLinks().back().port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
	assert(rt != nullptr);

	if (textureResource != rt->attachedTexture) {
		textureResource = rt->attachedTexture;
		Flag().fetch_or(RenderStage::TINY_MODIFIED, std::memory_order_relaxed);
	}
}

uint32_t RenderPortTextureInput::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) {
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
				barrier.srcStageMask = IRender::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // TODO:
				barrier.srcAccessMask = IRender::ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = IRender::ACCESS_SHADER_READ_BIT;
				barrier.aspectMask = (IRender::AspectFlagBits)(IRender::ASPECT_DEPTH_BIT | IRender::ASPECT_STENCIL_BIT);
				barrier.oldLayout = IRender::LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				barrier.newLayout = IRender::LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			} else {
				barrier.srcStageMask = IRender::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				barrier.srcAccessMask = IRender::ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = IRender::ACCESS_SHADER_READ_BIT;
				barrier.aspectMask = IRender::ASPECT_COLOR_BIT;
				barrier.oldLayout = IRender::LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				barrier.newLayout = IRender::LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			barrier.dstStageMask = IRender::PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // TOOD: read framebuffer texture on VS?
			barrier.baseMipLevel = 0;
			barrier.levelCount = 1;
			barrier.baseArrayLayer = 0;
			barrier.layerCount = 1;
			barrier.dependencyMask = IRender::DEPENDENCY_DEFAULT;

			render.SetupBarrier(queue, &barrier);
			return 1;
		}
	}

	return 0;
}
