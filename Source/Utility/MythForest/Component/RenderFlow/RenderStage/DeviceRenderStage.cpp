#include "DeviceRenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"

using namespace PaintsNow;

DeviceRenderStage::DeviceRenderStage(const String& config) : InputColor(renderTargetDescription.colorStorages[0]), transferResource(nullptr), transferQueue(nullptr) {
	Flag().fetch_or(RENDERSTAGE_ENABLED, std::memory_order_relaxed);

	transferDescription.copyColor = 1;
	transferDescription.copyDepth = 0;
	transferDescription.copyStencil = 0;
	transferDescription.reserved = 0;
	transferDescription.sourceIndex = 0;
	transferDescription.targetIndex = 0;
	transferDescription.sourceMipLevel = 0;
	transferDescription.targetMipLevel = 0;;
	transferDescription.sourceBaseArrayLayer = 0;
	transferDescription.targetBaseArrayLayer = 0;
	transferDescription.sourceLayerCount = 1;
	transferDescription.targetLayerCount = 1;
	transferDescription.source = nullptr;
	transferDescription.sourceRegion = Int2Pair(Int2(0, 0), Int2(0, 0));
	transferDescription.target = nullptr;
	transferDescription.targetRegion = Int2Pair(Int2(0, 0), Int2(0, 0));
}

TObject<IReflect>& DeviceRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
	}

	return *this;
}

void DeviceRenderStage::OnFrameResolutionUpdate(Engine& engine, IRender::Queue* queue, UShort2 res) {
	transferDescription.sourceRegion.second = Int2(res.x(), res.y());
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

void DeviceRenderStage::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	IRender::Resource::TransferDescription* desc = static_cast<IRender::Resource::TransferDescription*>(render.MapResource(queue, transferResource, 0));
	*desc = transferDescription;
	desc->source = static_cast<RenderStage*>(InputColor.GetLinks().back().port->GetNode())->GetRenderTargetResource();
	render.UnmapResource(queue, transferResource, IRender::MAP_DATA_EXCHANGE);
}

void DeviceRenderStage::PreInitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = render.GetQueueDevice(queue);
	transferResource = render.CreateResource(device, IRender::Resource::RESOURCE_TRANSFER);
	transferQueue = render.CreateQueue(device, IRender::QUEUE_REPEATABLE);

	// BaseClass::PreInitialize(engine, queue);
}

void DeviceRenderStage::Uninitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	render.DeleteResource(queue, transferResource);
	render.DeleteQueue(transferQueue);

	BaseClass::Uninitialize(engine, queue);
}

uint32_t DeviceRenderStage::OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& queues, IRender::Queue* instantMainQueue) {
	IRender& render = engine.interfaces.render;
	render.ExecuteResource(transferQueue, transferResource);
	render.FlushQueue(transferQueue);
	queues.emplace_back(transferQueue);

	return 0;
}
