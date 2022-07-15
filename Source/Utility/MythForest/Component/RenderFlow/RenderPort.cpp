#include "RenderPort.h"

using namespace PaintsNow;

TObject<IReflect>& RenderPort::operator () (IReflect& reflect) {
	typedef GraphPort<SharedTiny> Base;
	BaseClass::operator () (reflect);

	return *this;
}

bool RenderPort::OnFrameEncodeBegin(Engine& engine) {
	return true;
}

void RenderPort::OnFrameEncodeEnd(Engine& engine) {}

void RenderPort::OnFramePreTick(Engine& engine, IRender::Queue* queue) {
	eventTickHooks(engine, *this, queue);
}

void RenderPort::OnFrameUpdate(Engine& engine, IRender::Queue* queue) {}
void RenderPort::OnFramePostTick(Engine& engine, IRender::Queue* queue) {}
uint32_t RenderPort::OnSetupRenderTarget(Engine& engine, IRender::Queue* queue) { return 0; }
void RenderPort::Initialize(Engine& engine, IRender::Queue* mainQueue) {}
void RenderPort::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {}

void RenderPort::UpdateRenderStage() {
	Tiny* renderStage = GetNode();
	if (renderStage != nullptr) {
		renderStage->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}
}

uint32_t RenderPort::OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& repeatQueues, IRender::Queue* instantMainQueue) { return 0; }
