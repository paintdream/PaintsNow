#include "stdafx.h"
using namespace PaintsNow;

// Worker
Worker::Worker(Kernel& ker, IScript& script, uint32_t warp) : BaseClass(ker.GetThreadPool(), script), kernel(ker)
{
}

Worker::~Worker()
{
	if (compileRoutine)
	{
		script.DoLock();
		IScript::Request& request = script.GetDefaultRequest();
		request.Dereference(compileRoutine);
		script.UnLock();
	}
}

void Worker::ErrorHandler(IScript::Request& request, const String& err)
{
	if (errorHandler)
	{
		errorHandler(*this, request, err);
	}
}

void Worker::Dispatch(IScript::RequestPool* fromPool, IScript::RequestPool* toPool, ITask* task)
{
	Worker* worker = static_cast<Worker*>(toPool);
	kernel.QueueRoutine(worker, task);
}

