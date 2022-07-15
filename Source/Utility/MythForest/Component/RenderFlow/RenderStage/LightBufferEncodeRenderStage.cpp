#include "LightBufferEncodeRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include "../../../../SnowyStream/Manager/RenderResourceManager.h"
#include <sstream>

using namespace PaintsNow;

LightBufferEncodeRenderStage::LightBufferEncodeRenderStage(const String& config) : lastBufferLength(0) {
	uint8_t shift = Math::Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
	resolutionShift = Char2((char)shift, (char)shift);
}

TObject<IReflect>& LightBufferEncodeRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(InputDepth);
		ReflectProperty(LightSource);
		ReflectProperty(LightBuffer);
	}

	return *this;
}

// do not setup renderstate/rendertarget
uint32_t LightBufferEncodeRenderStage::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) { return 0; }

void LightBufferEncodeRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;

	assert(LightBuffer.sharedBufferResource == nullptr);
	IRender& render = engine.interfaces.render;
	IRender::Resource* resource = render.CreateResource(engine.snowyStream.GetRenderResourceManager()->GetRenderDevice(), IRender::Resource::RESOURCE_BUFFER);
	IRender::Resource::BufferDescription& description = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, resource, 0));
	description.state.format = IRender::Resource::BufferDescription::UNSIGNED_INT;
	description.state.component = 4;
	description.state.dynamic = 1;
	description.state.usage = IRender::Resource::BufferDescription::STORAGE; // shared storage
	render.UnmapResource(queue, resource, IRender::MAP_DATA_EXCHANGE);

	LightBuffer.sharedBufferResource = resource;
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	BaseClass::PreInitialize(engine, queue);
}

void LightBufferEncodeRenderStage::Uninitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	BaseClass::Uninitialize(engine, queue);
}

void LightBufferEncodeRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	LightBufferEncodePass& Pass = GetPass();
	LightEncoderCS& encoder = Pass.encoder;
	encoder.depthTexture.resource = InputDepth.textureResource->GetRenderResource();
	encoder.inverseProjectionParams = CameraView->inverseProjectionParams;

	// Prepare lights
	const std::vector<RenderPortLightSource::LightElement>& lights = LightSource.lightElements;
	// get depth texture size
	const UShort3& dim = InputDepth.textureResource->description.dimension;
	UInt3 computeGroup = GetPass().encoder.computeGroup;
	assert(computeGroup.z() == 1);

	// prepare soft rasterizer
	Float4* lightInfos = &encoder.lightInfos[0];
	int32_t gridWidth = (dim.x() + computeGroup.x() - 1) / computeGroup.x(), gridHeight = (dim.y() + computeGroup.y() - 1) / computeGroup.y();
	uint32_t count = Math::Min((uint32_t)lights.size(), (uint32_t)LightEncoderCS::MAX_LIGHT_COUNT);
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

	// need update light buffer?
	uint32_t length = gridWidth * gridHeight * computeGroup.x() * computeGroup.y() * sizeof(uint32_t) * 64;
	if (length != lastBufferLength) {
		IRender& render = engine.interfaces.render;
		IRender::Resource::BufferDescription& bufferDescription = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, LightBuffer.sharedBufferResource, 0));
		bufferDescription.state.usage = IRender::Resource::BufferDescription::STORAGE;
		bufferDescription.state.component = 1;
		bufferDescription.state.dynamic = 1;
		bufferDescription.state.format = IRender::Resource::BufferDescription::UNSIGNED_INT;
		bufferDescription.state.length = length;
		render.UnmapResource(queue, LightBuffer.sharedBufferResource, IRender::MAP_DATA_EXCHANGE);
		lastBufferLength = length;
	}

	encoder.lightBuffer.resource = LightBuffer.sharedBufferResource; // Binding output buffer.
	drawCallDescription.instanceCounts = UInt3(gridWidth, gridHeight, 1);
	LightBuffer.depthSize = UShort2(dim.x(), dim.y());
	LightBuffer.bufferSize = UShort2(gridWidth * computeGroup.x(), gridHeight * computeGroup.y());
	encoder.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());

	// assemble block data
	BaseClass::OnFrameUpdate(engine, queue);
}
