#include "EventGraphComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(EventGraphComponentModule);
EventGraphComponentModule::EventGraphComponentModule(Engine& engine) : BaseClass(engine) {}
EventGraphComponentModule::~EventGraphComponentModule() {}

TObject<IReflect>& EventGraphComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestQueueEntity)[ScriptMethodLocked = "QueueEntity"];
		ReflectMethod(RequestConnectEntity)[ScriptMethodLocked = "ConnectEntity"];
		ReflectMethod(RequestSetPollDelay)[ScriptMethodLocked = "SetPollDelay"];
	}

	return *this;
}

TShared<EventGraphComponent> EventGraphComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<EventGraphComponent> eventGraphComponent = TShared<EventGraphComponent>::From(allocator->New(std::ref(engine)));
	eventGraphComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return eventGraphComponent;
}

int32_t EventGraphComponentModule::RequestQueueEntity(IScript::Request& request, IScript::Delegate<EventGraphComponent> graph, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);
	CHECK_DELEGATE(entity);

	return graph->QueueEntity(entity.Get());
}

void EventGraphComponentModule::RequestConnectEntity(IScript::Request& request, IScript::Delegate<EventGraphComponent> graph, int32_t prev, int32_t next) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->Connect(prev, next);
}

void EventGraphComponentModule::RequestSetPollDelay(IScript::Request& request, IScript::Delegate<EventGraphComponent> graph, uint32_t delay) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->SetPollDelay(delay);
}
