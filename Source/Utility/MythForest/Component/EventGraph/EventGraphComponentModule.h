// EventGraphComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "EventGraphComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class EventGraphComponent;
	class EventGraphComponentModule : public TReflected<EventGraphComponentModule, ModuleImpl<EventGraphComponent> > {
	public:
		EventGraphComponentModule(Engine& engine);
		~EventGraphComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create EventGraphComponent
		/// </summary>
		/// <returns> EventGraphComponent object </returns>
		TShared<EventGraphComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Queue a new task to an existing task graph
		/// </summary>
		/// <param name="graph"> EventGraphComponent object </param>
		/// <param name="entity"> task tiny object </param>
		/// <returns> Queued task id </returns>
		int32_t RequestQueueEntity(IScript::Request& request, IScript::Delegate<EventGraphComponent> graph, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Set execution dependency between two tasks.
		/// </summary>
		/// <param name="graph"> EventGraphComponent object </param>
		/// <param name="prev"> pre task </param>
		/// <param name="next"> post task </param>
		/// <returns></returns>
		void RequestConnectEntity(IScript::Request& request, IScript::Delegate<EventGraphComponent> graph, int32_t prev, int32_t next);

		/// <summary>
		/// Set poll delay for task graph
		/// </summary>
		/// <param name="graph"> EventGraphComponent object </param>
		/// <param name="delay"> poll delay (in milliseconds) </param>
		/// <returns></returns>
		void RequestSetPollDelay(IScript::Request& request, IScript::Delegate<EventGraphComponent> graph, uint32_t delay);
	};
}

