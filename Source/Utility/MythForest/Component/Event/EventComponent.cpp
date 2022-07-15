#include "EventComponent.h"
#include "../../Entity.h"
#include "../../../HeartVioliner/Clock.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

EventComponent::EventComponent() : rootEntity(nullptr), tickTimeStamp(0), tickTimeDelta(0), frameTickerFrames(frameTickerCollection) {
	Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_relaxed);
}

bool EventComponent::ShouldTrigger(Engine& engine) const {
	bool trigger = (Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED) && !engine.bridgeSunset.IsExiting();
	if (trigger) {
		assert(rootEntity != nullptr);
	}

	return trigger;
}

Tiny::FLAG EventComponent::GetEntityFlagMask() const {
	return 0;
}

void EventComponent::Abort(void* context) {}

uint64_t EventComponent::GetTickDeltaTime() const {
	return tickTimeDelta;
}

Entity* EventComponent::GetHostEntity() const {
	return rootEntity;
}

void EventComponent::Execute(void* context) {
	Engine& engine = *reinterpret_cast<Engine*>(context);
	if (ShouldTrigger(engine)) {
		uint64_t time = ITimer::GetSystemClock();
		tickTimeDelta = time - tickTimeStamp;

		if (rootEntity->Flag().load(std::memory_order_relaxed) & Entity::ENTITY_HAS_TICK_EVENT) {
			Event event(engine, Event::EVENT_TICK, this, nullptr);
			event.counter = verify_cast<uint32_t>(tickTimeDelta);
			event.timestamp = tickTimeStamp;
			rootEntity->PostEvent(event, Entity::ENTITY_HAS_TICK_EVENT);
		}

		tickTimeStamp = time;
	}
}

void EventComponent::InstallFrame() {
	Flag().fetch_or(EVENTCOMPONENT_INSTALLED_FRAME, std::memory_order_relaxed);
}

void EventComponent::UninstallFrame() {
	Flag().fetch_and(~EVENTCOMPONENT_INSTALLED_FRAME, std::memory_order_relaxed);
}

void EventComponent::InstallTick(Engine& engine, const TShared<Clock>& c) {
	assert(c->GetWarpIndex() == GetWarpIndex());
	if (clock) {
		UninstallTick(engine);
	}

	if (!(Flag().load(std::memory_order_relaxed) & EVENTCOMPONENT_INSTALLED_TICK)) {
		Flag().fetch_or((EVENTCOMPONENT_INSTALLED_TICK | (EVENTCOMPONENT_BASE << Event::EVENT_TICK)), std::memory_order_release);

		clock = c;
		tickTimeStamp = ITimer::GetSystemClock();

		ReferenceObject();
		clock->AddTicker(this, &engine);
	}
}

void EventComponent::UninstallTick(Engine& engine) {
	if (Flag().load(std::memory_order_relaxed) & EVENTCOMPONENT_INSTALLED_TICK) {
		clock->RemoveTicker(this);
		clock = nullptr;
		Flag().fetch_and(~(EVENTCOMPONENT_INSTALLED_TICK | (EVENTCOMPONENT_BASE << Event::EVENT_TICK)), std::memory_order_release);

		ReleaseObject();
	}
}

void EventComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
	// enables all event slots ...
	assert(rootEntity == nullptr);
	rootEntity = entity;
}

void EventComponent::Uninitialize(Engine& engine, Entity* entity) {
	assert(rootEntity != nullptr);
	UninstallFrame();
	UninstallTick(engine);
	rootEntity = nullptr;
	BaseClass::Uninitialize(engine, entity);
}

void EventComponent::RoutineSetupFrameTickers(Engine& engine) {
	if (!(Flag().load(std::memory_order_relaxed) & EVENTCOMPONENT_INSTALLED_FRAME))
		return;

	if (rootEntity != nullptr) {
		Event eventSyncTickFrame(engine, Event::EVENT_FRAME_SYNC_TICK, this);
		size_t size = rootEntity->GetComponentCount();
		for (size_t i = 0; i < size; i++) {
			Component* component = rootEntity->GetComponent(i);
			if (component != nullptr && (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPECIAL_EVENT)) {
				component->DispatchEvent(eventSyncTickFrame, rootEntity);
				frameTickerFrames.Push(component);
			}
		}

		frameTickerFrames.Release();
	}
}

void EventComponent::RoutineOnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard) {
	engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventComponent::OnKeyboard), std::ref(engine), keyboard));
}

void EventComponent::RoutineOnMouse(Engine& engine, const IFrame::EventMouse& mouse) {
	engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventComponent::OnMouse), std::ref(engine), mouse));
}

void EventComponent::RoutineOnSize(Engine& engine, const Int2& size) {
	engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventComponent::OnSize), std::ref(engine), size));
}

void EventComponent::OnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard) {
	OPTICK_EVENT();
	if (ShouldTrigger(engine)) {
		if (rootEntity->Flag().load(std::memory_order_relaxed) & Entity::ENTITY_HAS_SPECIAL_EVENT) {
			Event event(engine, Event::EVENT_INPUT, this, TShared<SharedTiny>::From(new Event::Wrapper<IFrame::EventKeyboard>(keyboard)));
			rootEntity->PostEvent(event, Entity::ENTITY_HAS_SPECIAL_EVENT);
		}
	}
}

void EventComponent::OnMouse(Engine& engine, const IFrame::EventMouse& mouse) {
	OPTICK_EVENT();
	if (ShouldTrigger(engine)) {
		if (rootEntity->Flag().load(std::memory_order_relaxed) & Entity::ENTITY_HAS_SPECIAL_EVENT) {
			Event event(engine, Event::EVENT_INPUT, this, TShared<SharedTiny>::From(new Event::Wrapper<IFrame::EventMouse>(mouse)));
			rootEntity->PostEvent(event, Entity::ENTITY_HAS_SPECIAL_EVENT);
		}
	}
}

void EventComponent::OnSize(Engine& engine, const IFrame::EventSize& size) {
	OPTICK_EVENT();
	if (ShouldTrigger(engine)) {
		if (rootEntity->Flag().load(std::memory_order_relaxed) & Entity::ENTITY_HAS_SPECIAL_EVENT) {
			Event event(engine, Event::EVENT_INPUT, this, TShared<SharedTiny>::From(new Event::Wrapper<IFrame::EventSize>(size)));
			rootEntity->PostEvent(event, Entity::ENTITY_HAS_SPECIAL_EVENT);
		}
	}
}

void EventComponent::RoutineTickFrame(Engine& engine) {
	if (!(Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED))
		return;

	OPTICK_EVENT();

	Event event(engine, Event::EVENT_FRAME, rootEntity);
	// Do not post them directly because we are in render thread
	// rootEntity->PostEvent(event);

	if (rootEntity->Flag().load(std::memory_order_relaxed) & Entity::ENTITY_HAS_SPECIAL_EVENT) {
		if (Flag().load(std::memory_order_acquire) & EVENTCOMPONENT_INSTALLED_FRAME) {
			engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventComponent::RoutineSetupFrameTickers), std::ref(engine)));
			if (frameTickerFrames.AcquireCatchup()) {
				for (TQueueFrame<TQueueList<TShared<Component> > >::iterator it = frameTickerFrames.begin(); it != frameTickerFrames.end(); ++it) {
					(*it)->DispatchEvent(event, nullptr);
				}
			}
		}
	}
}

TObject<IReflect>& EventComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(rootEntity)[Runtime];
		ReflectProperty(frameTickerCollection)[Runtime];
		ReflectProperty(frameTickerFrames)[Runtime];
	}

	return *this;
}
