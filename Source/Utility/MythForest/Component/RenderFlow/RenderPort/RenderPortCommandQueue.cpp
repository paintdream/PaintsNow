#include "RenderPortCommandQueue.h"

using namespace PaintsNow;

RenderPortCommandQueue::RenderPortCommandQueue() : repeatQueue(nullptr), encodeCount(0) {}

RenderPortCommandQueue::~RenderPortCommandQueue() {
	assert(repeatQueue == nullptr);
}

TObject<IReflect>& RenderPortCommandQueue::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

bool RenderPortCommandQueue::OnFrameEncodeBegin(Engine& engine) {
	encodeCount++;
	if (CallbackFrameBegin) {
		CallbackFrameBegin(engine, *this);
	}

	return true;
}

IRender::Queue* RenderPortCommandQueue::GetRepeatQueue() const {
	return repeatQueue;
}

void RenderPortCommandQueue::OnFrameEncodeEnd(Engine& engine) {
	if (CallbackFrameEnd) {
		CallbackFrameEnd(engine, *this);
	}

	engine.interfaces.render.FlushQueue(repeatQueue);
	encodeCount++;
}

void RenderPortCommandQueue::CheckinState(IRender& render, IRender::Resource* stateResource) {
	render.ExecuteResource(repeatQueue, stateResource);
}

void RenderPortCommandQueue::DrawElement(IRender& render, IRender::Resource* drawCallResource) {
	render.ExecuteResource(repeatQueue, drawCallResource);
}

void RenderPortCommandQueue::Initialize(Engine& engine, IRender::Queue* mainQueue) {
	IRender& render = engine.interfaces.render;
	repeatQueue = render.CreateQueue(render.GetQueueDevice(mainQueue), IRender::QUEUE_REPEATABLE);
}

void RenderPortCommandQueue::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {
	IRender& render = engine.interfaces.render;
	if (repeatQueue != nullptr) {
		render.DeleteQueue(repeatQueue);
		repeatQueue = nullptr;
	}
}

uint32_t RenderPortCommandQueue::OnFrameCommit(Engine& engine, std::vector<IRender::Queue*>& repeatQueues, IRender::Queue* instantMainQueue) {
	IRender& render = engine.interfaces.render;
	uint32_t count = verify_cast<uint32_t>(instantQueues.size());
	for (size_t i = 0; i < instantQueues.size(); i++) {
		render.ExecuteResource(instantMainQueue, instantQueues[i]);
	}

	instantQueues.clear();
	repeatQueues.emplace_back(repeatQueue);
	return count;
}

void RenderPortCommandQueue::MergeQueue(IRender& render, IRender::Queue* queue, bool instant) {
	if (instant) {
		instantQueues.emplace_back(queue);
	} else {
		render.ExecuteResource(repeatQueue, queue);
	}
}

void RenderPortCommandQueue::OnFramePostTick(Engine& engine, IRender::Queue* mainQueue) {
	if (encodeCount == 0) {
		// not encoded by any external code, do it here
		if (engine.interfaces.render.IsQueueEmpty(repeatQueue)) {
			if (OnFrameEncodeBegin(engine)) {
				OnFrameEncodeEnd(engine);
			}
		}
	} else {
		assert(encodeCount % 2 == 0);
		encodeCount = 0;
	}

	BaseClass::OnFramePostTick(engine, mainQueue);
}
