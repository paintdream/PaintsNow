#include "DeferredLightingBufferEncodedRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

DeferredLightingBufferEncodedRenderStage::DeferredLightingBufferEncodedRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]), LoadDepth(renderTargetDescription.depthStorage) {
	renderStateDescription.stencilTest = IRender::Resource::RenderStateDescription::NEVER;
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

TObject<IReflect>& DeferredLightingBufferEncodedRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(LightSource);

		ReflectProperty(BaseColorOcclusion);
		ReflectProperty(NormalRoughnessMetallic);
		ReflectProperty(Depth);
		ReflectProperty(ShadowTexture);
		ReflectProperty(LastInputColor);
		ReflectProperty(ReflectCoord);

		ReflectProperty(LightBuffer);
		ReflectProperty(LoadDepth);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void DeferredLightingBufferEncodedRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGB10PACK;
	OutputColor.renderTargetDescription.state.sample = IRender::Resource::TextureDescription::LINEAR;
	OutputColor.renderTargetDescription.state.addressU = OutputColor.renderTargetDescription.state.addressV = OutputColor.renderTargetDescription.state.addressW = IRender::Resource::TextureDescription::CLAMP;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}

void DeferredLightingBufferEncodedRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	DeferredLightingBufferEncodedPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.screenTransform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	DeferredCompactDecodeFS& compactDecode = Pass.deferredCompactDecode;
	compactDecode.BaseColorOcclusionTexture.resource = BaseColorOcclusion.textureResource->GetRenderResource();
	compactDecode.NormalRoughnessMetallicTexture.resource = NormalRoughnessMetallic.textureResource->GetRenderResource();
	compactDecode.DepthTexture.resource = Depth.textureResource->GetRenderResource();
	compactDecode.inverseProjectionMatrix = CameraView->inverseProjectionMatrix;
	compactDecode.ShadowTexture.resource = ShadowTexture.textureResource->GetRenderResource();
	compactDecode.ScreenTexture.resource = LastInputColor.textureResource->GetRenderResource();
	compactDecode.ReflectCoordTexture.resource = ReflectCoord.textureResource->GetRenderResource();

	if (renderStateDescription.stencilMask != LightSource->stencilMask) {
		renderStateDescription.stencilMask = LightSource->stencilMask;
		renderStateDescription.stencilValue = LightSource->stencilMask;
		renderStateDescription.stencilTest = LightSource->stencilMask != 0 ? IRender::Resource::RenderStateDescription::EQUAL : IRender::Resource::RenderStateDescription::NEVER;
		renderStateDescription.stencilWrite = 0;
		IRender& render = engine.interfaces.render;
		IRender::Resource::RenderStateDescription& desc = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(queue, renderState, 0));
		desc = renderStateDescription;
		render.UnmapResource(queue, renderState, IRender::MAP_DATA_EXCHANGE);
	}

	StandardLightingBufferEncodedFS& standardLighting = Pass.standardLighting;
	standardLighting.cubeLevelInv = 1.0f;
	standardLighting.cubeStrength = 1.0f;

	if (LightSource->cubeMapTexture) {
		standardLighting.specTexture.resource = LightSource->cubeMapTexture->GetRenderResource();
		standardLighting.cubeLevelInv = 1.0f / Math::Log2x((uint32_t)LightSource->cubeMapTexture->description.dimension.x());
		standardLighting.cubeStrength = LightSource->cubeStrength;
	} else {
		standardLighting.specTexture.resource = BaseColorOcclusion.textureResource->GetRenderResource();
	}

	/*
	if (LightSource->skyMapTexture) {
		standardLighting.ambientTexture.resource = LightSource->skyMapTexture ? LightSource->skyMapTexture->GetTexture() : LightSource->cubeMapTexture->GetTexture();
	} else {
		// Temporary code, fail back
		standardLighting.ambientTexture.resource = standardLighting.specTexture.resource;
	}*/

	// fill light buffers
	standardLighting.invWorldNormalMatrix = CameraView->inverseViewMatrix;
	MatrixFloat3x3 normalMatrix;

	for (uint32_t j = 0; j < 3; j++) {
		for (uint32_t i = 0; i < 3; i++) {
			normalMatrix(i, j) = CameraView->viewMatrix(i, j);
		}
	}

	const std::vector<RenderPortLightSource::LightElement>& lights = LightSource->lightElements;
	uint32_t count = Math::Min((uint32_t)lights.size(), (uint32_t)StandardLightingBufferEncodedFS::MAX_LIGHT_COUNT);
	for (uint32_t i = 0; i < count; i++) {
		const RenderPortLightSource::LightElement& light = lights[i];

		Float3 p(light.position.x(), light.position.y(), light.position.z());
		if (light.position.w() != 0) {
			p = Math::Transform(CameraView->viewMatrix, p);
		} else {
			p = Math::Normalize(p * normalMatrix);
		}

		standardLighting.lightInfos[i * 2] = Float4(p.x(), p.y(), p.z(), light.position.w());
		standardLighting.lightInfos[i * 2 + 1] = light.colorAttenuation;
	}

	standardLighting.lightCount = count;
	assert(LightBuffer.sharedBufferResource);
	standardLighting.lightIndexBuffer.resource = LightBuffer.sharedBufferResource;
	standardLighting.lightBufferSize = Float2(LightBuffer.bufferSize.x(), LightBuffer.bufferSize.y());
	standardLighting.depthTextureSize = Float2(LightBuffer.depthSize.x(), LightBuffer.depthSize.y());

	BaseClass::OnFrameUpdate(engine, queue);
}
