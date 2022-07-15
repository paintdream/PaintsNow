#include "ScreenSpaceTraceRenderStage.h"

using namespace PaintsNow;

ScreenSpaceTraceRenderStage::ScreenSpaceTraceRenderStage(const String& s) : ScreenCoord(renderTargetDescription.colorStorages[0]), LoadDepth(renderTargetDescription.depthStorage) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	renderStateDescription.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
	renderStateDescription.depthWrite = 0;
	renderStateDescription.stencilTest = 1;
	renderStateDescription.stencilWrite = 0;
}


TObject<IReflect>& ScreenSpaceTraceRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Depth);
		ReflectProperty(LoadDepth);
		ReflectProperty(Normal);
		ReflectProperty(LightSource);
		ReflectProperty(CameraView);
		ReflectProperty(ScreenCoord);
	}

	return *this;
}

void ScreenSpaceTraceRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;

	ScreenCoord.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_SHORT;
	ScreenCoord.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RG;
	ScreenCoord.renderTargetDescription.state.sample = IRender::Resource::TextureDescription::POINT;
	ScreenCoord.renderTargetDescription.state.immutable = false;
	ScreenCoord.renderTargetDescription.state.attachment = true;

	BaseClass::PreInitialize(engine, queue);
}

void ScreenSpaceTraceRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	ScreenSpaceTracePass& pass = GetPass();
	pass.shaderScreen.depthTexture.resource = Depth.textureResource->GetRenderResource();
	pass.shaderScreen.normalTexture.resource = Normal.textureResource->GetRenderResource();
	pass.shaderScreen.projectionParams = CameraView->projectionParams;
	pass.shaderScreen.inverseProjectionParams = CameraView->inverseProjectionParams;
	pass.screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;

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

	const UShort3& dim = Depth.textureResource->description.dimension;
	pass.shaderScreen.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());

	BaseClass::OnFrameUpdate(engine, queue);
}
