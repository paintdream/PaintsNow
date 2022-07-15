#include "WidgetRenderStage.h"

using namespace PaintsNow;

WidgetRenderStage::WidgetRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]), InputColor(renderTargetDescription.colorStorages[0]) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	
	Widgets.CallbackFrameBegin = Wrap(this, &WidgetRenderStage::OnFrameEncodeBegin);
}

TObject<IReflect>& WidgetRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Widgets);
		ReflectProperty(InputColor);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void WidgetRenderStage::OnFrameEncodeBegin(Engine& engine, RenderPortCommandQueue& commandQueue) {
	IRender::Queue* queue = commandQueue.GetRepeatQueue();
	BaseClass::OnSetupRenderTarget(engine, queue);
}

uint32_t WidgetRenderStage::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) { return 0; }

void WidgetRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	if (InputColor.GetLinks().empty()) {
		InputColor.bindingStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	}

	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;

	BaseClass::PreInitialize(engine, queue);
}

void WidgetRenderStage::Initialize(Engine& engine, IRender::Queue* queue) {
	BaseClass::Initialize(engine, queue);
	IRender& render = engine.interfaces.render;
	// Initialize Repeat Command Buffer content
	if (Widgets.OnFrameEncodeBegin(engine)) {
		Widgets.OnFrameEncodeEnd(engine);
	}
}

void WidgetRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFrameUpdate(engine, queue);
}
