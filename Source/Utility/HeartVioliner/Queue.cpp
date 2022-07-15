#include "Queue.h"

using namespace PaintsNow;

Queue::Queue() {}

void Queue::Attach(const TShared<Clock>& c) {
	Detach();
	clock = c;
	clock->AddTicker(this, nullptr);
}

void Queue::Detach() {
	if (clock) {
		clock->RemoveTicker(this);
		clock = nullptr;
	}
}

void Queue::Clear(IScript::Request& request) {
	while (!q.empty()) {
		Task task = q.top();
		request.Dereference(task.ref);
		q.pop();
	}
}

void Queue::ScriptUninitialize(IScript::Request& request) {
	Clear(request);
	
	for (std::list<IScript::Request::Ref>::reverse_iterator it = listeners.rbegin(); !(it == listeners.rend()); ++it) {
		request.Dereference(*it);
	}

	listeners.clear();

	SharedTiny::ScriptUninitialize(request);
}

void Queue::Listen(IScript::Request& request, const IScript::Request::Ref& listener) {
	listeners.emplace_back(listener);
}

void Queue::Push(IScript::Request& request, IScript::Request::Ref& ref, int64_t timeStamp) {
	q.push(Task(ref, timeStamp));
}

void Queue::Execute(void* context) {
	int64_t timeStamp = clock->Now();
	BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
	IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
	ExecuteWithTimeStamp(request, timeStamp);
	bridgeSunset.requestPool.ReleaseSafe(&request);
}

void Queue::Abort(void* context) {}

void Queue::ExecuteWithTimeStamp(IScript::Request& request, int64_t timeStamp) {
	while (!q.empty()) {
		Task task = q.top();
		if (task.timeStamp > timeStamp) break;

		q.pop();
		Post(request, task.ref, timeStamp);
	}
}

void Queue::Post(IScript::Request& request, IScript::Request::Ref ref, int64_t timeStamp) {
	// make a function call directly
	for (std::list<IScript::Request::Ref>::reverse_iterator it = listeners.rbegin(); !(it == listeners.rend()); ++it) {
		request.Push();
		request.Call(*it, timeStamp, ref);
		request.Pop();
		assert(request.GetScript()->IsLocked());
	}

	// release the reference
	request.Dereference(ref);
	assert(request.GetScript()->IsLocked());
}