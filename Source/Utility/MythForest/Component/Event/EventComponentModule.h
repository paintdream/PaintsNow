// EventComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#pragma once
#include "EventComponent.h"
#include "../../Module.h"
#include "../../../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	class Clock;
	class Connection;
	class EventComponent;
	class EventComponentModule : public TReflected<EventComponentModule, ModuleImpl<EventComponent> > {
	public:
		EventComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void TickFrame() override;
		void OnSize(const Int2& size);
		void OnMouse(const IFrame::EventMouse& mouse);
		void OnKeyboard(const IFrame::EventKeyboard& keyboard);
		void Uninitialize() override;

		/// <summary>
		/// Create EventComponent
		/// </summary>
		/// <returns> EventComponent object </returns>
		TShared<EventComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Bind a Clock to an EventComponent, driving the tick event.
		/// </summary>
		/// <param name="eventComponent"> the EventComponent </param>
		/// <param name="clock"> the Clock to bind </param>
		void RequestBindEventTick(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<Clock> clock);

		/// <summary>
		/// Enable render device frame tick for EventComponent
		/// </summary>
		/// <param name="eventComponent"> the EventComponent </param>
		/// <param name="enable"> enable render frame tick or not </param>
		void RequestBindEventFrame(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool enable);

		/// <summary>
		/// Bind a connection to EventComponent, driving network events.
		/// </summary>
		/// <param name="eventComponent"> the EventComponent </param>
		/// <param name="connection"> the Connection </param>
		void RequestBindEventNetwork(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<Connection> connection);

		/// <summary>
		/// Enable user input (mouse/keyboard/...) event for EventComponent
		/// </summary>
		/// <param name="eventComponent"> the EventComponent </param>
		/// <param name="enable"> enable user input events or not </param>
		void RequestBindEventUserInput(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool enable);

		/// <summary>
		/// Set event filter for EventComponent
		/// </summary>
		/// <param name="eventComponent"> the EventComponent </param>
		/// <param name="idMask"> event mask </param>
		void RequestFilterEvent(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, uint32_t idMask);

	protected:
		std::vector<TShared<EventComponent> > frameTickers;
		std::vector<TShared<EventComponent> > userInputs;
		std::atomic<size_t> critical;
	};
}

