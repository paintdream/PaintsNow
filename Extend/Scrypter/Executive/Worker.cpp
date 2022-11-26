#include "stdafx.h"
using namespace PaintsNow;

// Worker
Worker::Worker(Kernel& ker, IScript& script, uint32_t warp) : BaseClass(ker.GetThreadPool(), script), kernel(ker), currentDocument(nullptr), crossScriptModule(ker.GetThreadPool(), script), currentThreadId(0), errorCount(0)
{
	SetWarpIndex(warp);
	Flag().fetch_or(CROSSSCRIPT_TRANSPARENT, std::memory_order_relaxed);
}

Worker::~Worker()
{
	Clear();
}

void Worker::Clear()
{
	if (compileRoutine)
	{
		script.DoLock();
		IScript::Request& request = script.GetDefaultRequest();
		request.Dereference(compileRoutine);
		script.UnLock();
	}

	requestPool.Clear();
	script.Reset();
}

void Worker::ErrorHandler(IScript::Request& request, const String& err)
{
	errorCount++;
	currentDocument->WriteLog(err);
}

struct DocumentContextTaskWrapper : public TaskOnce
{
	DocumentContextTaskWrapper(ITask* t, Worker* w, Document* d) : baseTask(t), worker(w), document(d)
	{}

	void Execute(void* context) override
	{
		TShared<Document> stash = worker->currentDocument;
		worker->currentDocument = document;
		worker->currentThreadId = ::GetCurrentThreadId();
		document->PrepareCall();
		baseTask->Execute(context);
		document->CompleteCall();
		worker->currentThreadId = 0;
		worker->currentDocument = stash;

		ITask::Delete(this);
	}

	ITask* baseTask;
	TShared<Worker> worker;
	TShared<Document> document;
};

void Worker::Dispatch(IScript::RequestPool* fromPool, IScript::RequestPool* toPool, ITask* task)
{
	Worker* worker = static_cast<Worker*>(toPool);
	kernel.QueueRoutine(worker, new (ITask::Allocate(sizeof(DocumentContextTaskWrapper))) DocumentContextTaskWrapper(task, worker, currentDocument()));
}

void Worker::Queue(const TShared<Document>& document, IScript::Request::Ref callback)
{
	TShared<Document> stash = currentDocument;
	currentDocument = document;
	currentThreadId = ::GetCurrentThreadId();

	IScript::Request& req = *requestPool.AcquireSafe();
	req.DoLock();
	req.Push();
	req.Call(callback);
	req.Pop();
	req.Dereference(callback);
	req.UnLock();
	requestPool.ReleaseSafe(&req);

	currentThreadId = 0;
	currentDocument = stash;
}

