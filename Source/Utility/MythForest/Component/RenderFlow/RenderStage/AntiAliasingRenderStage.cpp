#include "AntiAliasingRenderStage.h"

using namespace PaintsNow;

AntiAliasingRenderStage::AntiAliasingRenderStage(const String& options) : OutputColor(renderTargetDescription.colorStorages[0]) {}

TObject<IReflect>& AntiAliasingRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);

		ReflectProperty(InputColor);
		ReflectProperty(LastInputColor);
		ReflectProperty(Depth);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void AntiAliasingRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGB10PACK;
	OutputColor.renderTargetDescription.state.sample = IRender::Resource::TextureDescription::LINEAR;
	OutputColor.renderTargetDescription.state.addressU = OutputColor.renderTargetDescription.state.addressV = OutputColor.renderTargetDescription.state.addressW = IRender::Resource::TextureDescription::MIRROR_CLAMP;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}

void AntiAliasingRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	// Do not update pass in Update(engine)
	AntiAliasingPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	
	AntiAliasingFS& antiAliasing = Pass.antiAliasing;
	antiAliasing.inputTexture.resource = InputColor.textureResource->GetRenderResource();
	antiAliasing.lastInputTexture.resource = LastInputColor.textureResource->GetRenderResource();
	assert(antiAliasing.inputTexture.resource != antiAliasing.lastInputTexture.resource);
	// assert(OutputColor.renderTargetTextureResource->GetTexture() != antiAliasing.lastInputTexture.resource);
	antiAliasing.depthTexture.resource = Depth.textureResource->GetRenderResource();
	antiAliasing.reprojectionMatrix = CameraView->reprojectionMatrix;
	// antiAliasing.inverseProjectionParams = CameraView->inverseProjectionParams;
	antiAliasing.unjitter = CameraView->jitterOffset * 0.5f;
	antiAliasing.lastRatio = CameraView->jitterHistoryRatio;

	const UShort3& dim = InputColor.textureResource->description.dimension;
	antiAliasing.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());
	// std::swap(antiAliasing.unjitter, antiAliasing.invScreenSize);

	BaseClass::OnFrameUpdate(engine, queue);
}

