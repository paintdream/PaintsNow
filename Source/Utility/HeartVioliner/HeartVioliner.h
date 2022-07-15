// HeartVioliner.h -- Operation queue
// PaintDream (paintdream@paintdream.com)
// 2015-1-3
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITimer.h"
#include "Queue.h"
#include "Clock.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	class HeartVioliner : public TReflected<HeartVioliner, IScript::Library> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;
		// static int main(int argc, char* argv[]);
		HeartVioliner(IThread& thread, ITimer& factory, BridgeSunset& bridgeSunset);

	protected:
		/// <summary>
		/// Create a clock
		/// </summary>
		/// <param name="interval"> Specifies the clock interval in milliseconds</param>
		/// <returns> Clock object </returns>
		TShared<Clock> RequestNewClock(IScript::Request& request, int64_t interval, int64_t start);

		/// <summary>
		/// Force set a clock current time
		/// </summary>
		/// <param name="clock"> Specifies the clock object </param>
		/// <param name="time"> Specified new current time </param>
		void RequestSetClock(IScript::Request& request, IScript::Delegate<Clock> clock, int64_t time);

		/// <summary>
		/// Attach queue to clock
		/// </summary>
		/// <param name="clock"> Specifies the clock object </param>
		/// <param name="queue"> Specifies the queue object </param>
		/// <returns> </returns>
		void RequestAttach(IScript::Request& request, IScript::Delegate<Clock> clock, IScript::Delegate<Queue> queue);

		/// <summary>
		/// Detach queue from clock
		/// </summary>
		/// <param name="queue"> Specifies the queue object </param>
		/// <returns> </returns>
		void RequestDetach(IScript::Request& request, IScript::Delegate<Queue> queue);

		/// <summary>
		/// Start clock
		/// </summary>
		/// <param name="clock"> Specifies the clock to start </param>
		/// <returns> </returns>
		void RequestStart(IScript::Request& request, IScript::Delegate<Clock> clock);

		/// <summary>
		/// Pause clock
		/// </summary>
		/// <param name="clock"> Specifies the clock to pause </param>
		/// <returns> </returns>
		void RequestPause(IScript::Request& request, IScript::Delegate<Clock> clock);

		/// <summary>
		/// Get current clock time
		/// </summary>
		/// <param name="clock"> Specifies the clock object </param>
		/// <returns> The current time of the clock specified. </returns>
		int64_t RequestNow(IScript::Request& request, IScript::Delegate<Clock> clock);

		/// <summary>
		/// Create a priority queue.
		/// </summary>
		/// <returns> Queue object </returns>
		TShared<Queue> RequestNewQueue(IScript::Request& request);

		/// <summary>
		/// Create a listener/recorder/logger on specified queue.
		/// Multiple listener/recorder/loggers on the same queue are also accepted.
		/// Listener prototype: listener(package)
		/// </summary>
		/// <param name="queue"> Specifies the queue </param>
		/// <param name="listener"> Specifies the listener </param>
		/// <returns> </returns>
		void RequestListen(IScript::Request& request, IScript::Delegate<Queue> queue, IScript::Request::Ref listener);

		/// <summary>
		/// Push an element to specified queue.
		/// </summary>
		/// <param name="queue">Specifies the queue to push</param>
		/// <param name="timeStamp">Specifies the key (timeStamp)</param>
		/// <param name="element">Specifies the element</param>
		/// <returns> </returns>
		void RequestPush(IScript::Request& request, IScript::Delegate<Queue> queue, int64_t timeStamp, IScript::Request::Ref element);

		/// <summary>
		/// Pop all elements whose timeStamp is less than timeStamp specified.
		/// </summary>
		/// <param name="queue">Specifies the queue to pop</param>
		/// <param name="timeStamp">Specifies the key (timeStamp)</param>
		/// <returns> </returns>
		void RequestPop(IScript::Request& request, IScript::Delegate<Queue> queue, int64_t timeStamp);

		/// <summary>
		/// Pop/drop all elements
		/// </summary>
		/// <param name="queue"> Specifies the queue to pop</param>
		/// <returns> </returns>
		void RequestClear(IScript::Request& request, IScript::Delegate<Queue> queue);

	private:
		ITimer& timerFactory;
		BridgeSunset& bridgeSunset;
	};
}

