#include "LightTextureEncodeRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

LightTextureEncodeRenderStage::LightTextureEncodeRenderStage(const String& config) : LightTexture(renderTargetDescription.colorStorages[0]) {
	uint8_t shift = Math::Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
	resolutionShift = Char2((char)shift, (char)shift);
}

TObject<IReflect>& LightTextureEncodeRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(InputDepth);
		ReflectProperty(LightSource);
		ReflectProperty(LightTexture);
	}

	return *this;
}

void LightTextureEncodeRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	LightTexture.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	LightTexture.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	LightTexture.renderTargetDescription.state.sample = IRender::Resource::TextureDescription::POINT;
	LightTexture.renderTargetDescription.state.immutable = false;
	LightTexture.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}

void LightTextureEncodeRenderStage::Uninitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	BaseClass::Uninitialize(engine, queue);
}

void LightTextureEncodeRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	LightTextureEncodePass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	LightEncoderFS& encoder = Pass.encoder;
	encoder.depthTexture.resource = InputDepth.textureResource->GetRenderResource();
	encoder.inverseProjectionParams = CameraView->inverseProjectionParams;

	// Prepare lights
	const std::vector<RenderPortLightSource::LightElement>& lights = LightSource.lightElements;
	// get depth texture size
	const UShort3& dim = InputDepth.textureResource->description.dimension;
	// prepare soft rasterizer
	Float4* lightInfos = &encoder.lightInfos[0];
	uint32_t count = Math::Min((uint32_t)lights.size(), (uint32_t)LightEncoderFS::MAX_LIGHT_COUNT);
	encoder.lightCount = (float)count;

	MatrixFloat3x3 normalMatrix;

	for (uint32_t j = 0; j < 3; j++) {
		for (uint32_t i = 0; i < 3; i++) {
			normalMatrix(i, j) = CameraView->viewMatrix(i, j);
		}
	}

	for (uint32_t i = 0; i < count; i++) {
		const RenderPortLightSource::LightElement& light = lights[i];
		Float3 p(light.position.x(), light.position.y(), light.position.z());
		if (light.position.w() != 0) {
			p = Math::Transform(CameraView->viewMatrix, p);
		} else {
			p = Math::Normalize(p * normalMatrix);
		}

		lightInfos[i] = Float4(p.x(), p.y(), p.z(), light.position.w());
	}

	// assemble block data
	BaseClass::OnFrameUpdate(engine, queue);
}
