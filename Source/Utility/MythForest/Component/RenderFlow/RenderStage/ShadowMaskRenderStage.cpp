#include "ShadowMaskRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ShadowMaskRenderStage::ShadowMaskRenderStage(const String& config) : OutputMask(renderTargetDescription.colorStorages[0]), InputMask(renderTargetDescription.colorStorages[0]), LoadDepth(renderTargetDescription.depthStorage), MoveDepth(renderTargetDescription.depthStorage) {
	layerIndex = atoi(config.c_str());
	IRender::Resource::RenderStateDescription& s = renderStateDescription;
	s.cullFrontFace = 1;
	s.stencilTest = IRender::Resource::RenderStateDescription::GREATER;
	s.stencilWrite = 1;
	s.stencilReplacePass = 1;
	s.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
	s.depthWrite = 0;

	renderTargetDescription.depthStorage.loadOp = renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

TObject<IReflect>& ShadowMaskRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(CameraView);
		ReflectProperty(InputDepth);
		ReflectProperty(LoadDepth);
		ReflectProperty(MoveDepth);
		ReflectProperty(InputMask);
		ReflectProperty(OutputMask);
	}

	return *this;
}

void ShadowMaskRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;

	if (InputMask.GetLinks().empty()) {
		OutputMask.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		OutputMask.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::R;
		OutputMask.renderTargetDescription.state.immutable = false;
		OutputMask.renderTargetDescription.state.attachment = true;

		renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	} else {
		renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	}

	emptyShadowMask = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "[Runtime]/TextureResource/Black", true, ResourceBase::RESOURCE_VIRTUAL);

	const String path = "[Runtime]/MeshResource/StandardCube";
	meshResource = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);

	BaseClass::PreInitialize(engine, queue);
}

void ShadowMaskRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	ShadowMaskPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	ShadowMaskFS& mask = Pass.mask;
	mask.depthTexture.resource = InputDepth.textureResource->GetRenderResource();
	mask.shadowTexture.resource = emptyShadowMask->GetRenderResource();

	if (renderStateDescription.stencilMask != LightSource->stencilShadow) {
		renderStateDescription.stencilMask = renderStateDescription.stencilValue = LightSource->stencilShadow;
		IRender& render = engine.interfaces.render;
		IRender::Resource::RenderStateDescription& desc = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(queue, renderState, 0));
		desc = renderStateDescription;
		render.UnmapResource(queue, renderState, IRender::MAP_DATA_EXCHANGE);
	}

	MatrixFloat4x4 inverseMatrix = CameraView->inverseProjectionMatrix * CameraView->inverseViewMatrix;

	for (size_t i = 0; i < LightSource->lightElements.size(); i++) {
		RenderPortLightSource::LightElement& element = LightSource->lightElements[i];
		// just get first shadow
		if (layerIndex < element.shadows.size()) {
			RenderPortLightSource::LightElement::Shadow& shadow = element.shadows[layerIndex];
			mask.reprojectionMatrix = inverseMatrix * shadow.shadowMatrix;
			const UShort3& dim = InputDepth.textureResource->description.dimension;
			mask.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());
			mask.unjitter = CameraView->jitterOffset * 0.5f;
			screenTransform.worldTransform = Math::QuickInverse(shadow.shadowMatrix) * CameraView->viewMatrix * CameraView->projectionMatrix;

			if (shadow.shadowTexture) {
				mask.shadowTexture.resource = shadow.shadowTexture->GetRenderResource();
			}
		}
	}

	// assert(LightSource->lightElements.empty() || mask.shadowTexture.resource != emptyShadowMask->GetTexture());
	BaseClass::OnFrameUpdate(engine, queue);
}
