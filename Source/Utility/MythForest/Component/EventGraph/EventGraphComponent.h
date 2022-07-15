// EventGraphComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../../Core/System/TaskGraph.h"

namespace PaintsNow {
	class EventGraphComponent : public TAllocatedTiny<EventGraphComponent, Component>, public TaskRepeat {
	public:
		EventGraphComponent(Engine& engine);
		~EventGraphComponent() override;

		/*
		enum {
			EVENTGRAPHCOMPONENT_SYNCRONIZED = COMPONENT_CUSTOM_BEGIN,
			EVENTGRAPHCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1,
		};*/

		int32_t QueueEntity(Entity* entity);
		void Connect(int32_t prev, int32_t next);
		void SetPollDelay(uint32_t delay);

		void Execute(void* context) override;
		void Abort(void* context) override;
		void DispatchEvent(Event& event, Entity* entity) override;

	protected:
		void OnTaskGraphFinished();

	protected:
		Event* currentEvent;
		std::atomic<uint32_t> finished;
		uint32_t pollDelay;
		TShared<TaskGraph> taskGraph;
	};
}

