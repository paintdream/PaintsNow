#include "ScreenRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ScreenRenderStage::ScreenRenderStage(const String& config) : OutputColor(renderTargetDescription.colorStorages[0]) {
	size_t count = Math::Max(1, atoi(config.c_str()));
	BloomLayers.resize(count);
	for (size_t i = 0; i < count; i++) {
		BloomLayers[i] = TShared<RenderPortTextureInput>::From(new RenderPortTextureInput());
	}
}

TObject<IReflect>& ScreenRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	
	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
		ReflectProperty(BloomLayers);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void ScreenRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}

void ScreenRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	ScreenPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.screenTransform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	ScreenFS& ScreenFS = Pass.shaderScreen;
	ScreenFS.inputColorTexture.resource = InputColor.textureResource->GetRenderResource();
	assert(BloomLayers.size() > 0);
	
	ScreenFS.inputBloomTexture0.resource = BloomLayers[0]->textureResource->GetRenderResource();
	ScreenFS.inputBloomTexture1.resource = BloomLayers.size() > 1 ? BloomLayers[1]->textureResource->GetRenderResource() : ScreenFS.inputBloomTexture0.resource;
	ScreenFS.inputBloomTexture2.resource = BloomLayers.size() > 2 ? BloomLayers[2]->textureResource->GetRenderResource() : ScreenFS.inputBloomTexture1.resource;

	BaseClass::OnFrameUpdate(engine, queue);
}
