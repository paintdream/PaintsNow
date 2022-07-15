#include "DepthBoundingSetupRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

DepthBoundingSetupRenderStage::DepthBoundingSetupRenderStage(const String& config) : OutputDepth(renderTargetDescription.colorStorages[0]) {
	uint8_t shift = Math::Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
	resolutionShift = Char2((char)shift, (char)shift);
}

TObject<IReflect>& DepthBoundingSetupRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputDepth);
		ReflectProperty(OutputDepth);
	}

	return *this;
}

void DepthBoundingSetupRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputDepth.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputDepth.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RG;
	OutputDepth.renderTargetDescription.state.sample = IRender::Resource::TextureDescription::POINT;
	OutputDepth.renderTargetDescription.state.immutable = false;
	OutputDepth.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}

void DepthBoundingSetupRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	DepthBoundingSetupPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	DepthMinMaxSetupFS& minmax = Pass.minmax;
	minmax.depthTexture.resource = InputDepth.textureResource->GetRenderResource();
	const UShort3& dim = OutputDepth.renderTargetDescription.dimension;
	minmax.invScreenSize = Float2(dim.x() == 0 ? 0 : 1.0f / dim.x(), dim.y() == 0 ? 0 : 1.0f / dim.y());
	BaseClass::OnFrameUpdate(engine, queue);
}
