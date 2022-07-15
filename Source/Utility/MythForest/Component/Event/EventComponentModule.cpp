#include "EventComponentModule.h"
#include "../../Engine.h"
#include "../../../HeartVioliner/Clock.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include <iterator>

using namespace PaintsNow;

CREATE_MODULE(EventComponentModule);
EventComponentModule::EventComponentModule(Engine& engine) : BaseClass(engine) {
	critical.store(0, std::memory_order_relaxed);
}

TObject<IReflect>& EventComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestBindEventTick)[ScriptMethodLocked = "BindEventTick"];
		ReflectMethod(RequestBindEventFrame)[ScriptMethodLocked = "BindEventFrame"];
		ReflectMethod(RequestBindEventUserInput)[ScriptMethodLocked = "BindEventUserInput"];
		ReflectMethod(RequestBindEventNetwork)[ScriptMethodLocked = "BindEventNetwork"];
		ReflectMethod(RequestFilterEvent)[ScriptMethodLocked = "FilterEvent"];
	}

	if (reflect.IsReflectProperty()) {
		ReflectProperty(frameTickers)[Runtime];
		ReflectProperty(userInputs)[Runtime];
	}

	if (reflect.IsReflectEnum()) {
		Event::ReflectEventIDs(reflect);
	}

	return *this;
}

TShared<EventComponent> EventComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<EventComponent> eventComponent = TShared<EventComponent>::From(allocator->New());
	eventComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return eventComponent;
}

void EventComponentModule::RequestBindEventTick(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	if (clock) {
		eventComponent->InstallTick(engine, clock.Get());
	} else {
		eventComponent->UninstallTick(engine);
	}
}

void EventComponentModule::RequestFilterEvent(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, uint32_t mask) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	uint32_t flag, old;
	do {
		flag = old = eventComponent->Flag().load(std::memory_order_acquire);
		flag = (flag & ~(EventComponent::EVENTCOMPONENT_ALL)) | (mask * EventComponent::EVENTCOMPONENT_BASE & EventComponent::EVENTCOMPONENT_ALL);
	} while (!eventComponent->Flag().compare_exchange_weak(old, flag, std::memory_order_release));
}

void EventComponentModule::RequestBindEventFrame(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool add) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	if (add) {
		eventComponent->InstallFrame();

		TSpinLockGuard<size_t> guard(critical);
		std::vector<TShared<EventComponent> >::iterator it = std::find(frameTickers.begin(), frameTickers.end(), eventComponent.Get());
		if (it == frameTickers.end()) frameTickers.emplace_back(eventComponent.Get());
	} else {
		TSpinLockGuard<size_t> guard(critical);
		std::vector<TShared<EventComponent> >::iterator it = std::find(frameTickers.begin(), frameTickers.end(), eventComponent.Get());
		if (it != frameTickers.end()) frameTickers.erase(it);

		eventComponent->UninstallFrame();
	}
}

void EventComponentModule::TickFrame() {
	std::vector<TShared<EventComponent> > nextTickedListeners;
	do {
		TSpinLockGuard<size_t> guard(critical);
		nextTickedListeners = frameTickers;
	} while (false);

	for (size_t j = 0; j < nextTickedListeners.size(); j++) {
		nextTickedListeners[j]->RoutineTickFrame(engine);
	}
}

void EventComponentModule::OnSize(const Int2& size) {
	std::vector<TShared<EventComponent> > nextUserInputs;
	do {
		TSpinLockGuard<size_t> guard(critical);
		nextUserInputs = userInputs;
	} while (false);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnSize(engine, size);
	}
}

void EventComponentModule::OnMouse(const IFrame::EventMouse& mouse) {
	std::vector<TShared<EventComponent> > nextUserInputs;
	do {
		TSpinLockGuard<size_t> guard(critical);
		nextUserInputs = userInputs;
	} while (false);


	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnMouse(engine, mouse);
	}
}

void EventComponentModule::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	std::vector<TShared<EventComponent> > nextUserInputs;
	do {
		TSpinLockGuard<size_t> guard(critical);
		nextUserInputs = userInputs;
	} while (false);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnKeyboard(engine, keyboard);
	}
}

void EventComponentModule::RequestBindEventUserInput(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool enable) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	if (enable) {
		eventComponent->Flag().fetch_or((EventComponent::EVENTCOMPONENT_BASE << Event::EVENT_INPUT), std::memory_order_relaxed);
		TSpinLockGuard<size_t> guard(critical);
		std::vector<TShared<EventComponent> >::iterator it = BinaryFind(userInputs.begin(), userInputs.end(), eventComponent.Get());
		if (it == userInputs.end()) BinaryInsert(userInputs, eventComponent.Get());
	} else {
		TSpinLockGuard<size_t> guard(critical);
		std::vector<TShared<EventComponent> >::iterator it = BinaryFind(userInputs.begin(), userInputs.end(), eventComponent.Get());
		if (it != userInputs.end()) userInputs.erase(it);
		eventComponent->Flag().fetch_and(~(EventComponent::EVENTCOMPONENT_BASE << Event::EVENT_INPUT), std::memory_order_release);
	}
}

void EventComponentModule::RequestBindEventNetwork(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<Connection> connection) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	// TODO:
}

void EventComponentModule::Uninitialize() {
	TSpinLockGuard<size_t> guard(critical);
	for (size_t i = 0; i < frameTickers.size(); i++) {
		frameTickers[i]->UninstallFrame();
	}

	frameTickers.clear();
	userInputs.clear();
}