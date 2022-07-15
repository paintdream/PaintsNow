#include "ScreenOutlineRenderStage.h"

using namespace PaintsNow;

ScreenOutlineRenderStage::ScreenOutlineRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]), InputColor(renderTargetDescription.colorStorages[0]) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

TObject<IReflect>& ScreenOutlineRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void ScreenOutlineRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;

	BaseClass::PreInitialize(engine, queue);
}

void ScreenOutlineRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFrameUpdate(engine, queue);
}
