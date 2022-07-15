#include "FrameBarrierRenderStage.h"

using namespace PaintsNow;

FrameBarrierRenderStage::FrameBarrierRenderStage(const String& s) : Next(renderTargetDescription.colorStorages[0]) {
	Flag().fetch_and(~RENDERSTAGE_ADAPT_MAIN_RESOLUTION, std::memory_order_release);
	Front.Flag().fetch_or(RenderStage::RENDERSTAGE_WEAK_LINKAGE, std::memory_order_relaxed);
	Next.renderTargetDescription.state.attachment = true;
}

TObject<IReflect>& FrameBarrierRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Front);
		ReflectProperty(Next);
	}

	return *this;
}

void FrameBarrierRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;

	// we do not have render targets
	// RenderStage::PreInitialize(engine);
}

void FrameBarrierRenderStage::OnFrameResolutionUpdate(Engine& engine, IRender::Queue* queue, UShort2 res) {
	if (Next.GetLinks().empty()) return;

	RenderStage* stage = static_cast<RenderStage*>(Next.GetLinks().back().port->GetNode());
	if (stage->Flag().load(std::memory_order_relaxed) & RENDERSTAGE_ADAPT_MAIN_RESOLUTION) {
		Flag().fetch_or(RENDERSTAGE_ADAPT_MAIN_RESOLUTION, std::memory_order_relaxed);
		resolutionShift = stage->resolutionShift;

		BaseClass::OnFrameResolutionUpdate(engine, queue, res);
	} else {
		Flag().fetch_and(~RENDERSTAGE_ADAPT_MAIN_RESOLUTION, std::memory_order_relaxed);
	}
}

void FrameBarrierRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {}
uint32_t FrameBarrierRenderStage::OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& queues, IRender::Queue* instantMainQueue) { return 0; }

void FrameBarrierRenderStage::OnFrameTick(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFrameTick(engine, queue);
	if (Front.GetLinks().empty()) return;

	if (!Next.attachedTexture ||
		Front.textureResource->description.state != Next.renderTargetDescription.state
		|| Front.textureResource->description.dimension.x() != Next.renderTargetDescription.dimension.x()
		|| Front.textureResource->description.dimension.y() != Next.renderTargetDescription.dimension.y()) {

		Next.renderTargetDescription.state = Front.textureResource->description.state;
		assert(Next.renderTargetDescription.state.attachment);
		Next.renderTargetDescription.dimension.x() = Front.textureResource->description.dimension.x();
		Next.renderTargetDescription.dimension.y() = Front.textureResource->description.dimension.y();
		if (!Next.attachedTexture) {
			Next.attachedTexture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("Swap", this), false, ResourceBase::RESOURCE_VIRTUAL);
		}

		Next.attachedTexture->description.state = Next.renderTargetDescription.state;
		Next.attachedTexture->description.dimension = Next.renderTargetDescription.dimension;
		Next.attachedTexture->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
		Next.attachedTexture->GetResourceManager().InvokeUpload(Next.attachedTexture(), queue);
	}

	RenderPortRenderTargetStore* rt = Front.GetLinks().back().port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());

	rt->attachedTexture = Next.attachedTexture;
	Next.attachedTexture = Front.textureResource;

	rt->Flag().fetch_or(TINY_MODIFIED | RENDERSTAGE_RENDERTARGET_MODIFIED, std::memory_order_relaxed);
}

