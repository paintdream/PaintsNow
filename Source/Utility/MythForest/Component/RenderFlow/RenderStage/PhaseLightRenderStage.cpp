#include "PhaseLightRenderStage.h"

using namespace PaintsNow;

PhaseLightRenderStage::PhaseLightRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]) {
	renderStateDescription.blend = 1;
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

TObject<IReflect>& PhaseLightRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(InputColor);
		ReflectProperty(Depth);
		ReflectProperty(BaseColorOcclusion);
		ReflectProperty(NormalRoughnessMetallic);
		ReflectProperty(PhaseLightView);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void PhaseLightRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	BaseClass::PreInitialize(engine, queue);
}

void PhaseLightRenderStage::OnFrameTick(Engine& engine, IRender::Queue* queue) {
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	BaseClass::OnFrameTick(engine, queue);
}

void PhaseLightRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	const std::vector<RenderPortPhaseLightView::PhaseInfo>& phases = PhaseLightView.phases;
	MultiHashTracePass& pass = GetPass();
	MultiHashTraceFS& fs = pass.shaderMultiHashTrace;
	fs.dstDepthTexture.resource = Depth.textureResource->GetRenderResource();
	fs.dstBaseColorOcclusionTexture.resource = BaseColorOcclusion.textureResource->GetRenderResource();
	fs.dstNormalRoughnessMetallicTexture.resource = NormalRoughnessMetallic.textureResource->GetRenderResource();
	fs.dstInverseProjection = CameraView->inverseProjectionMatrix;

	for (size_t i = 0; i < phases.size(); i++) {
		const RenderPortPhaseLightView::PhaseInfo& phase = phases[i];
		// Set params
		fs.srcProjection = CameraView->inverseViewMatrix * phase.viewMatrix * phase.projectionMatrix;
		fs.srcInverseProjection = Math::Inverse(fs.srcProjection);
		UShort3& dim = phase.irradiance->description.dimension;
		Float2 inv(1.0f / dim.x(), 1.0f / dim.y());

		// generate offsets
		for (size_t i = 0; i < fs.offsets.size(); i++) {
			fs.offsets[i] = Float2((float)rand() / (float)RAND_MAX * inv.x(), (float)rand() / (float)RAND_MAX * inv.y());
		}

		BaseClass::OnFrameUpdate(engine, queue);
	}
}
