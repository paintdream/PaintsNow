// ReflectTrail.h
// PaintDream (paintdream@paintdream.com)
// 2021-04-04
//

#pragma once

#include "LeavesTrail.h"
#include "../../../Interface/IReflect.h"

namespace PaintsNow {
	class ReflectTrail : public LeavesTrail {
	public:
		ReflectTrail(IThread& threadApi, const IReflectObjectComplex& prototype, size_t maxEventCount = DEFAULT_EVENT_COUNT, size_t tickMultiplier = 1, size_t maxSleepCount = 256);
		~ReflectTrail();

		bool OnThreadTick(IThread::Thread* thread, size_t index);

	protected:
		IThread& thread;
		IThread::Thread* tickThread;
		size_t tickMultiplier;
		size_t maxSleepCount;
	};
}