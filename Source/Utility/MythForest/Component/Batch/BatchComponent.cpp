#include "BatchComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../MythForest/MythForest.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"

using namespace PaintsNow;

BatchComponent::BatchComponent(IRender::Resource::BufferDescription::Usage usage) : buffer(nullptr), bufferUsage(usage) {
	referenceCount.store(0, std::memory_order_relaxed);
	critical.store(0, std::memory_order_relaxed);
}

BatchComponent::~BatchComponent() {
	assert(referenceCount.load(std::memory_order_acquire) == 0);
}

void BatchComponent::InstanceInitialize(Engine& engine) {
	if (referenceCount.fetch_add(1, std::memory_order_relaxed) == 0) {
		IRender& render = engine.interfaces.render;
		buffer = render.CreateResource(engine.snowyStream.GetRenderResourceManager()->GetRenderDevice(), IRender::Resource::RESOURCE_BUFFER);
	}
}

void BatchComponent::InstanceUninitialize(Engine& engine) {
	if (referenceCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
		IRender& render = engine.interfaces.render;
		IRender::Queue* queue = engine.snowyStream.GetRenderResourceManager()->GetWarpResourceQueue();
		render.DeleteResource(queue, buffer);
		currentData = std::vector<uint8_t>();

		Flag().fetch_and(~Tiny::TINY_MODIFIED, std::memory_order_release);
	}
}

IRender::Resource::BufferDescription::Usage BatchComponent::GetBufferUsage() const {
	return bufferUsage;
}

uint32_t BatchComponent::Update(IRender& render, IRender::Queue* queue) {
	if (Flag().fetch_and(~Tiny::TINY_MODIFIED) & Tiny::TINY_MODIFIED) {
		IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, buffer, 0));
		desc.state.component = 4; // packed by float4
		desc.state.format = IRender::Resource::BufferDescription::FLOAT;
		desc.state.dynamic = 0;
		desc.state.usage = IRender::Resource::BufferDescription::UNIFORM;
		do {
			TSpinLockGuard<uint32_t> guard(critical);
			desc.data.Resize(currentData.size());
			if (!currentData.empty()) {
				desc.data.Import(0, &currentData[0], currentData.size());
			}
		} while (false);

		render.UnmapResource(queue, buffer, IRender::MAP_DATA_EXCHANGE);

		return 1;
	} else {
		return 0;
	}
}

IRender::Resource::DrawCallDescription::BufferRange BatchComponent::AllocateSafe(const void* data, uint32_t appendSize) {
	TSpinLockGuard<uint32_t> guard(critical);
	return Allocate(data, appendSize);
}

IRender::Resource::DrawCallDescription::BufferRange BatchComponent::Allocate(const void* data, uint32_t appendSize) {
	assert(appendSize != 0);
	assert(buffer != nullptr); // Must call InstanceInitialize() first!
	size_t curSize = currentData.size();
	currentData.resize(curSize + appendSize);
	memcpy(&currentData[0] + curSize, data, appendSize);

	IRender::Resource::DrawCallDescription::BufferRange bufferRange;
	bufferRange.buffer = buffer;
	bufferRange.offset = verify_cast<uint32_t>(curSize);
	bufferRange.length = appendSize;
	bufferRange.component = 0;
	bufferRange.type = 0;
	Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);

	return bufferRange;
}
