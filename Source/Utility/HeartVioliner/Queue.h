// Queue.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-4
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITimer.h"
#include "../../Core/System/Kernel.h"
#include "../../Core/Interface/IType.h"
#include "Clock.h"
#include <queue>
#include <list>

namespace PaintsNow {
	class Queue : public TReflected<Queue, WarpTiny>, public TaskRepeat {
	public:
		Queue();
		void Attach(const TShared<Clock>& clock);
		void Detach();

		void Listen(IScript::Request& request, const IScript::Request::Ref& listener);
		void Push(IScript::Request& request, IScript::Request::Ref& ref, int64_t timeStamp);
		void ScriptUninitialize(IScript::Request& request) override;
		void Clear(IScript::Request& request);
		void Execute(void* context) override;
		void Abort(void* context) override;
		void ExecuteWithTimeStamp(IScript::Request& request, int64_t timeStamp);

	protected:
		void Post(IScript::Request& request, IScript::Request::Ref ref, int64_t timeStamp);

	private:
		class Task {
		public:
			Task(const IScript::Request::Ref& r, int64_t t) : timeStamp(t), ref(r) {}
			int64_t timeStamp;
			IScript::Request::Ref ref;
			bool operator < (const Task& task) const {
				return timeStamp > task.timeStamp;
			}
		};

		std::priority_queue<Task> q;
		std::list<IScript::Request::Ref> listeners;
		TShared<Clock> clock;
	};
}
