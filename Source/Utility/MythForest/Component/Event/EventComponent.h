// EventComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#pragma once
#include "../../../../Core/Template/TEvent.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../Component.h"

namespace PaintsNow {
	class Clock;
	class EventComponent : public TAllocatedTiny<EventComponent, Component>, public TaskRepeat {
	public:
		EventComponent();

		void RoutineOnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard);
		void RoutineOnMouse(Engine& engine, const IFrame::EventMouse& mouse);
		void RoutineOnSize(Engine& engine, const Int2& size);
		void RoutineTickFrame(Engine& engine);

		void OnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard);
		void OnMouse(Engine& engine, const IFrame::EventMouse& mouse);
		void OnSize(Engine& engine, const IFrame::EventSize& size);
		void InstallTick(Engine& engine, const TShared<Clock>& clock);
		void UninstallTick(Engine& engine);
		void InstallFrame();
		void UninstallFrame();

		enum {
			EVENTCOMPONENT_UPDATING_FRAMELIST = COMPONENT_CUSTOM_BEGIN,
			EVENTCOMPONENT_INSTALLED_FRAME = COMPONENT_CUSTOM_BEGIN << 1,
			EVENTCOMPONENT_INSTALLED_TICK = COMPONENT_CUSTOM_BEGIN << 2,
			EVENTCOMPONENT_BASE = COMPONENT_CUSTOM_BEGIN << 3,
			EVENTCOMPONENT_END = EVENTCOMPONENT_BASE << (Event::EVENT_END - Event::EVENT_SPECIAL_BEGIN),
			EVENTCOMPONENT_ALL = EVENTCOMPONENT_END - EVENTCOMPONENT_BASE,
			EVENTCOMPONENT_CUSTOM_BEGIN = EVENTCOMPONENT_END
		};

		Entity* GetHostEntity() const override;
		FLAG GetEntityFlagMask() const override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void Execute(void* context) override;
		void Abort(void* context) override;

		uint64_t GetTickDeltaTime() const;
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		bool ShouldTrigger(Engine& engine) const;
		// Since we can't iterate entity's components in render thread,
		// Apply an asynchronized way to collect components instead.
		void RoutineSetupFrameTickers(Engine& engines);

	protected:
		TQueueList<TShared<Component> > frameTickerCollection;
		TQueueFrame<TQueueList<TShared<Component> > > frameTickerFrames;

		TShared<Clock> clock;
		Entity* rootEntity;
		uint64_t tickTimeStamp;
		uint64_t tickTimeDelta;
	};
}

