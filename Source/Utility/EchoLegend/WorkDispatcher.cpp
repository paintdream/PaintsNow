#include "WorkDispatcher.h"

using namespace PaintsNow;

WorkDispatcher::WorkDispatcher(BridgeSunset& bridgeSunset, INetwork& network, ITunnel::Dispatcher* disp) : BaseClass(std::ref(bridgeSunset), std::ref(network)), dispatcher(disp) {}

WorkDispatcher::~WorkDispatcher() {
	assert(dispatcher != nullptr);
	network.DeactivateDispatcher(dispatcher);
	if (thread != nullptr) {
		IThread& threadApi = bridgeSunset.GetThreadApi();
		threadApi.Wait(thread);
		threadApi.DeleteThread(thread);
	}

	network.CloseDispatcher(dispatcher);
}

ITunnel::Dispatcher* WorkDispatcher::GetDispatcher() const {
	assert(dispatcher != nullptr);
	return dispatcher;
}

bool WorkDispatcher::Activate() {
	assert(dispatcher != nullptr);
	return network.ActivateDispatcher(dispatcher);
}

void WorkDispatcher::Deactivate() {
	network.DeactivateDispatcher(dispatcher);
}