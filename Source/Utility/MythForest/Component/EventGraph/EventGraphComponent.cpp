#include "EventGraphComponent.h"
#include "../../Entity.h"

using namespace PaintsNow;

EventGraphComponent::EventGraphComponent(Engine& engine) : currentEvent(nullptr), pollDelay(1) {
	finished.store(0, std::memory_order_relaxed);
	taskGraph = TShared<TaskGraph>::From(new TaskGraph(engine.GetKernel()));
}

EventGraphComponent::~EventGraphComponent() {}

void EventGraphComponent::SetPollDelay(uint32_t delay) {
	pollDelay = delay;
}

void EventGraphComponent::OnTaskGraphFinished() {
	currentEvent = nullptr;
	finished.store(1u, std::memory_order_release);
}

void EventGraphComponent::DispatchEvent(Event& event, Entity* entity) {
	currentEvent = &event;

	Engine& engine = event.engine;
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.GetThreadPool();

	uint32_t warpIndex = GetWarpIndex();
	assert(warpIndex == kernel.GetCurrentWarpIndex());
	WarpYieldGuard guard(kernel);
	
	finished.store(0, std::memory_order_release);
	taskGraph->Dispatch(Wrap(this, &EventGraphComponent::OnTaskGraphFinished));

	// synchronized: wait for all task finished!!
	kernel.Wait(finished, 1u, 1u, pollDelay);
}

void EventGraphComponent::Execute(void* context) {
	assert(currentEvent != nullptr);
	Entity* entity = reinterpret_cast<Entity*>(context);
	entity->PostEvent(*currentEvent, ~(Tiny::FLAG)0); // may be visited in parallel!
}

void EventGraphComponent::Abort(void* context) {
	Execute(context); // force execute!
}

int32_t EventGraphComponent::QueueEntity(Entity* entity) {
	assert(entity != nullptr);
	return verify_cast<int32_t>(taskGraph->Insert(entity, this, entity));
}

void EventGraphComponent::Connect(int32_t prev, int32_t next) {
	taskGraph->Next(prev, next);
}
