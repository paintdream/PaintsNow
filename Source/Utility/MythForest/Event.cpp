#include "Event.h"

#include <utility>

using namespace PaintsNow;

#if defined(_MSC_VER) && _MSC_VER <= 1200
Event::Event() : eventID(EVENT_TICK), counter(0), timestamp(0) {}
#endif

Event::Event(Engine& e, EVENT_ID id, const TShared<SharedTiny> sender, const TShared<SharedTiny>& d) : engine(e), eventID(id), counter(0), timestamp(0), sender(std::move(sender)), detail(std::move(d)) {}
