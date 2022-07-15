#include "RenderPortSharedBuffer.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortSharedBufferLoad

RenderPortSharedBufferLoad::RenderPortSharedBufferLoad(bool write) : sharedBufferResource(nullptr) {
	if (write) {
		Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_relaxed);
	}
}

TObject<IReflect>& RenderPortSharedBufferLoad::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {}

	return *this;
}

void RenderPortSharedBufferLoad::Initialize(Engine& engine, IRender::Queue* mainQueue) {}
void RenderPortSharedBufferLoad::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {}

void RenderPortSharedBufferLoad::OnFramePreTick(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFramePreTick(engine, queue);
	if (GetLinks().empty()) return;

	RenderPortSharedBufferStore* bufferStore = GetLinks().back().port->QueryInterface(UniqueType<RenderPortSharedBufferStore>());
	if (bufferStore != nullptr) {
		if (sharedBufferResource != bufferStore->sharedBufferResource || bufferSize != bufferStore->bufferSize || depthSize != bufferStore->depthSize) {
			sharedBufferResource = bufferStore->sharedBufferResource;
			bufferSize = bufferStore->bufferSize;
			depthSize = bufferStore->depthSize;

			Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
		}
	}
}

uint32_t RenderPortSharedBufferLoad::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) {
	if (GetLinks().empty())
		return 0;

	IRender& render = engine.interfaces.render;
	RenderPort* port = static_cast<RenderPort*>(GetLinks().back().port);
	RenderPortSharedBufferStore* target = port->QueryInterface(UniqueType<RenderPortSharedBufferStore>());
	if (target != nullptr) {
		IRender::Resource* buffer = target->sharedBufferResource;
		if (buffer != nullptr) {
			IRender::Barrier barrier; // buffer barrier
			barrier.resource = buffer;
			barrier.srcAccessMask = IRender::ACCESS_MEMORY_WRITE_BIT;
			barrier.dstAccessMask = IRender::ACCESS_MEMORY_READ_BIT; // TODO: rw by default
			barrier.srcStageMask = IRender::PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			barrier.dstStageMask = IRender::PIPELINE_STAGE_TOP_OF_PIPE_BIT; // TOOD: more precisely

			barrier.offset = 0;
			barrier.size = 0;
			barrier.dependencyMask = IRender::DEPENDENCY_DEFAULT;

			render.SetupBarrier(queue, &barrier);
			return 1;
		}
	}

	return 0;
}

// RenderPortSharedBufferStore

RenderPortSharedBufferStore::RenderPortSharedBufferStore() : sharedBufferResource(nullptr) {
}

TObject<IReflect>& RenderPortSharedBufferStore::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}

void RenderPortSharedBufferStore::Initialize(Engine& engine, IRender::Queue* mainQueue) {}

void RenderPortSharedBufferStore::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {
	if (sharedBufferResource != nullptr) {
		IRender& render = engine.interfaces.render;
		render.DeleteResource(mainQueue, sharedBufferResource);
		sharedBufferResource = nullptr;
	}
}

void RenderPortSharedBufferStore::OnFramePreTick(Engine& engine, IRender::Queue* queue) {
	BaseClass::OnFramePreTick(engine, queue);
}

