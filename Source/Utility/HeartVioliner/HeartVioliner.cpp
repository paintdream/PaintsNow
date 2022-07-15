#include "HeartVioliner.h"
#include "Queue.h"
#include "Clock.h"

using namespace PaintsNow;

HeartVioliner::HeartVioliner(IThread& thread, ITimer& base, BridgeSunset& b) : timerFactory(base), bridgeSunset(b) {}

TObject<IReflect>& HeartVioliner::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewClock)[ScriptMethodLocked = "NewClock"];
		ReflectMethod(RequestSetClock)[ScriptMethodLocked = "SetClock"];
		ReflectMethod(RequestAttach)[ScriptMethodLocked = "Attach"];
		ReflectMethod(RequestDetach)[ScriptMethodLocked = "Detach"];
		ReflectMethod(RequestStart)[ScriptMethodLocked = "Start"];
		ReflectMethod(RequestPause)[ScriptMethodLocked = "Pause"];
		ReflectMethod(RequestNow)[ScriptMethodLocked = "Now"];
		ReflectMethod(RequestNewQueue)[ScriptMethodLocked = "NewQueue"];
		ReflectMethod(RequestListen)[ScriptMethodLocked = "Listen"];
		ReflectMethod(RequestPush)[ScriptMethodLocked = "Push"];
		ReflectMethod(RequestPop)[ScriptMethodLocked = "Pop"];
		ReflectMethod(RequestClear)[ScriptMethodLocked = "Clear"];
	}

	return *this;
}

TShared<Queue> HeartVioliner::RequestNewQueue(IScript::Request& request) {
	TShared<Queue> q = TShared<Queue>::From(new Queue());
	q->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
	return q;
}

void HeartVioliner::RequestStart(IScript::Request& request, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);

	clock->Play();
}

void HeartVioliner::RequestPause(IScript::Request& request, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);
	clock->Pause();
}

int64_t HeartVioliner::RequestNow(IScript::Request& request, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);
	return clock->Now();
}

void HeartVioliner::RequestPush(IScript::Request& request, IScript::Delegate<Queue> queue, int64_t timeStamp, IScript::Request::Ref obj) {
	CHECK_REFERENCES_LOCKED(obj);
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->Push(request, obj, timeStamp);
}

void HeartVioliner::RequestPop(IScript::Request& request, IScript::Delegate<Queue> queue, int64_t timeStamp) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->ExecuteWithTimeStamp(request, timeStamp);
}

void HeartVioliner::RequestClear(IScript::Request& request, IScript::Delegate<Queue> queue) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);
	queue->Clear(request);
}

void HeartVioliner::RequestListen(IScript::Request& request, IScript::Delegate<Queue> queue, IScript::Request::Ref listener) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(listener, IScript::Request::FUNCTION);
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);
	queue->Listen(request, listener);
}

TShared<Clock> HeartVioliner::RequestNewClock(IScript::Request& request, int64_t interval, int64_t start) {
	TShared<Clock> c = TShared<Clock>::From(new Clock(timerFactory, bridgeSunset, interval, start, true));
	c->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
	return c;
}

void HeartVioliner::RequestSetClock(IScript::Request& request, IScript::Delegate<Clock> clock, int64_t start) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);
	clock->SetClock(start);
}

void HeartVioliner::RequestAttach(IScript::Request& request, IScript::Delegate<Clock> clock, IScript::Delegate<Queue> queue) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(clock);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->Attach(clock.Get());
}

void HeartVioliner::RequestDetach(IScript::Request& request, IScript::Delegate<Queue> queue) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->Detach();
}

/*
#include "../../General/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../General/Driver/Script/Lua/ZScriptLua.h"
#include "../../General/Driver/Timer/GLFW/ZTimerGLFW.h"

int HeartVioliner::main(int argc, char* argv[]) {
	ZThreadPthread thread;
	ZScriptLua script(thread);
	TFactory<ZTimerGLFW, ITimer> fact;
	BridgeSunset bridgeSunset(thread, 3, 3);
	HeartVioliner hv(thread, fact, bridgeSunset);
	IScript::Request& request = script.GetDefaultRequest();
	IScript::Request::Ref ref = request.Load(String(
		"print('HeartVioliner::main()')\n"
		"local queue = HeartVioliner.CreateQueue()\n"
		"HeartVioliner.Listen(queue, function(v) print('Triggered! ' .. v) end)\n"
		"HeartVioliner.Push(queue, 0, 'hello')\n"
		"HeartVioliner.Push(queue, 1234, 'world')\n"
		"HeartVioliner.Push(queue, 3456, 1024)\n"
		"HeartVioliner.Pop(queue, 3333)\n"
		"print('STATUS' .. (queue == queue and 'YES' or 'NO'))\n"
		"print('IO' .. (io == nil and 'YES' or 'NO'))\n"
		));
	request << global << key("HeartVioliner") << hv << endtable;
	request << global << key("io") << nil << endtable;
	request << global << key("debug") << nil << endtable;
	request.Push();
	request.Call(ref);
	request.Pop();
	request.Dereference(ref);
	return 0;
}
*/