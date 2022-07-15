#include "ForwardLightingRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ForwardLightingRenderStage::ForwardLightingRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]), InputColor(renderTargetDescription.colorStorages[0]), InputDepth(renderTargetDescription.depthStorage), OutputDepth(renderTargetDescription.depthStorage) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	renderStateDescription.depthTest = IRender::Resource::RenderStateDescription::GREATER;
	renderStateDescription.depthWrite = 1;
	
	Primitives.CallbackFrameBegin = Wrap(this, &ForwardLightingRenderStage::OnFrameEncodeBegin);
}

TObject<IReflect>& ForwardLightingRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(Primitives);

		ReflectProperty(InputColor);
		ReflectProperty(InputDepth);
		ReflectProperty(OutputColor);
		ReflectProperty(OutputDepth);
	}

	return *this;
}

void ForwardLightingRenderStage::OnFrameEncodeBegin(Engine& engine, RenderPortCommandQueue& commandQueue) {
	IRender::Queue* queue = commandQueue.GetRepeatQueue();
	BaseClass::OnSetupRenderTarget(engine, queue);
}

uint32_t ForwardLightingRenderStage::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) { return 0; }

void ForwardLightingRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	bool supportIntegratedD24S8Format = render.GetProfile(render.GetQueueDevice(queue), "DepthFormat_D24S8") != 0;
	OutputDepth.renderTargetDescription.state.format = (uint32_t)(supportIntegratedD24S8Format ? IRender::Resource::TextureDescription::HALF : IRender::Resource::TextureDescription::FLOAT);
	OutputDepth.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::DEPTH_STENCIL;
	OutputDepth.renderTargetDescription.state.immutable = false;
	OutputDepth.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}
	
void ForwardLightingRenderStage::Initialize(Engine& engine, IRender::Queue* queue) {
	BaseClass::Initialize(engine, queue);
	// Initialize Repeat Command Buffer content
	if (Primitives.OnFrameEncodeBegin(engine)) {
		Primitives.OnFrameEncodeEnd(engine);
	}
}

void ForwardLightingRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFrameUpdate(engine, queue);
}
