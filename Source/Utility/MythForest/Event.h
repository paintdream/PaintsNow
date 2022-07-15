// Event.h
// PaintDream (paintdream@paintdream.com)
// 2015-9-12
//

#pragma once
#include "../../Core/System/Tiny.h"
#include "Engine.h"

namespace PaintsNow {
	class Component;
	class Event {
	public:
		enum EVENT_ID {
			// Inner events
			EVENT_BEGIN = 0,
			EVENT_TICK = EVENT_BEGIN,
			EVENT_UPDATE,
			EVENT_ATTACH_COMPONENT,
			EVENT_DETACH_COMPONENT,
			EVENT_ENTITY_ACTIVATE,
			EVENT_ENTITY_DEACTIVATE,
			// Special Events
			EVENT_SPECIAL_BEGIN,
			EVENT_FRAME = EVENT_SPECIAL_BEGIN,
			EVENT_FRAME_SYNC_TICK,
			EVENT_INPUT,
			EVENT_OUTPUT,
			EVENT_CUSTOM,
			EVENT_END
		};

		static void ReflectEventIDs(IReflect& reflect) {
			if (reflect.IsReflectEnum()) {
				ReflectEnum(EVENT_TICK);
				ReflectEnum(EVENT_UPDATE);
				ReflectEnum(EVENT_ATTACH_COMPONENT);
				ReflectEnum(EVENT_DETACH_COMPONENT);
				ReflectEnum(EVENT_ENTITY_ACTIVATE);
				ReflectEnum(EVENT_ENTITY_DEACTIVATE);
				ReflectEnum(EVENT_FRAME);
				ReflectEnum(EVENT_FRAME_SYNC_TICK);
				ReflectEnum(EVENT_INPUT);
				ReflectEnum(EVENT_OUTPUT);
				ReflectEnum(EVENT_CUSTOM);
				ReflectEnum(EVENT_END);
			}
		}

		template <class T>
		class Wrapper : public TReflected<Wrapper<T>, SharedTiny> {
		public:
			Wrapper() {}
			Wrapper(const T& d) : data(d) {}
			Wrapper(rvalue<T> d) { data = std::move(d); }

			T data;
		};

		Event(Engine& engine, EVENT_ID id, const TShared<SharedTiny> sender, const TShared<SharedTiny>& detail = nullptr);

#if defined(_MSC_VER) && _MSC_VER <= 1200
		Event();
#endif
		std::reference_wrapper<Engine> engine;	// (0/0)
		uint32_t eventID;
		uint32_t counter;
		uint64_t timestamp;
		TShared<SharedTiny> sender;
		TShared<SharedTiny> detail;
	};
}

