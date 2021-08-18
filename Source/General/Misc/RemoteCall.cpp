#include "RemoteCall.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"
using namespace PaintsNow;

RemoteCall::RemoteCall(IThread& thread, ITunnel& t, IFilterBase& f, const String& e, const TWrapper<void, bool, STATUS, const String&>& sh) : threadApi(thread), tunnel(t), filter(f), entry(e), statusHandler(sh), dispatcher(nullptr) {
	dispThread.store(nullptr, std::memory_order_release);
}

void RemoteCall::Stop() {
	if (dispatcher != nullptr) {
		tunnel.DeactivateDispatcher(dispatcher);

		IThread::Thread* thread = dispThread.exchange(nullptr, std::memory_order_release);
		if (thread != nullptr) {
			threadApi.Wait(thread);
			threadApi.DeleteThread(thread);
		}
	}
}

RemoteCall::~RemoteCall() {
	Stop();

	if (dispatcher != nullptr) {
		tunnel.CloseDispatcher(dispatcher);
	}
}

bool RemoteCall::ThreadProc(IThread::Thread* thread, size_t context) {
	OPTICK_THREAD("Network RemoteCall");

	ITunnel::Dispatcher* disp = dispatcher;
	ITunnel::Listener* listener = tunnel.OpenListener(disp, Wrap(this, &RemoteCall::HandleEvent), Wrap(this, &RemoteCall::OnConnection), entry);
	if (listener == nullptr) {
		HandleEvent(ITunnel::ABORT);
		return false;
	}

	tunnel.ActivateListener(listener);
	tunnel.ActivateDispatcher(disp); // running ...

	tunnel.DeactivateListener(listener);
	tunnel.CloseListener(listener);

	IThread::Thread* t = dispThread.exchange(nullptr, std::memory_order_release);
	if (t != nullptr) {
		assert(t == thread);
		threadApi.DeleteThread(t);
	}

	return false;
}

void RemoteCall::HandleEvent(ITunnel::EVENT event) {}

void RemoteCall::Reset() {
	Stop();
	Run();
}

bool RemoteCall::Run() {
	if (dispatcher == nullptr) {
		dispatcher = tunnel.OpenDispatcher();
	}

	assert(dispThread.load(std::memory_order_relaxed) == nullptr);
	dispThread.store(threadApi.NewThread(Wrap(this, &RemoteCall::ThreadProc), 0), std::memory_order_relaxed);

	return dispThread.load(std::memory_order_relaxed) != nullptr;
}

const ITunnel::Handler RemoteCall::OnConnection(ITunnel::Connection* connection) {
	// TODO:
	return nullptr;
}