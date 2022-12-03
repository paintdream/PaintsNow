#include "Looper.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

Looper::Looper(BridgeSunset& bs, INetwork& nt) : network(nt), bridgeSunset(bs), thread(nullptr) {}

Looper::~Looper() {
	/*
	if (thread != nullptr) {
		bridgeSunset.GetThreadApi().Wait(thread);
	}*/
}

class AsyncInfo {
public:
	AsyncInfo(IScript& s, Looper* l) : script(s), looper(l) {}
	IScript& script;
	Looper* looper;
};

bool Looper::ActivateRoutine(IThread::Thread* thread, size_t context) {
	OPTICK_THREAD("Network Looper");

	AsyncInfo* info = reinterpret_cast<AsyncInfo*>(context);
	Activate();
	delete info;

	return false;
}

void Looper::AsyncActivate(IScript::Request& request) {
	assert(thread == nullptr);
	// hold self reference
	IThread& threadApi = bridgeSunset.GetThreadApi();
	request.DoLock();
	AsyncInfo* info = new AsyncInfo(*request.GetScript(), this);
	request.UnLock();

	thread = threadApi.NewThread(Wrap(this, &Looper::ActivateRoutine), reinterpret_cast<size_t>(info));
	threadApi.SetThreadName(thread, "EchoLegend::Looper");
}

String Looper::EventToString(INetwork::EVENT event) {
	String target = "Close";
	switch (event) {
		case INetwork::CONNECTED:
			target = "Connected";
			break;
		case INetwork::TIMEOUT:
			target = "Timeout";
			break;
		case INetwork::READ:
			target = "Read";
			break;
		case INetwork::WRITE:
			target = "Write";
			break;
		case INetwork::CLOSE:
			target = "Close";
			break;
		case INetwork::ABORT:
			target = "Error";
			break;
		case INetwork::AWAKE:
			target = "Awake";
			break;
		case INetwork::CUSTOM:
			target = "Custom";
			break;
	}

	return target;
}
