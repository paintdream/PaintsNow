#include "BloomRenderStage.h"

using namespace PaintsNow;

BloomRenderStage::BloomRenderStage(const String& config) : OutputColor(renderTargetDescription.colorStorages[0]) {
	uint8_t shift = Math::Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
	resolutionShift = Char2((char)shift, (char)shift);
}

TObject<IReflect>& BloomRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
		ReflectProperty(OutputColor);
	}
	return *this;
}

void BloomRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGB10PACK;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}

void BloomRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	BloomPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.screenTransform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	
	BloomFS& bloom = Pass.screenBloom;
	bloom.screenTexture.resource = InputColor.textureResource->GetRenderResource();
	const UShort3& dim = OutputColor.renderTargetDescription.dimension;
	bloom.invScreenSize = Float2(dim.x() == 0 ? 0 : 1.0f / dim.x(), dim.y() == 0 ? 0 : 1.0f / dim.y());

	BaseClass::OnFrameUpdate(engine, queue);
}
