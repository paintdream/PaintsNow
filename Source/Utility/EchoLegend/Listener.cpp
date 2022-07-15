#include "Listener.h"
#include "Connection.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

Listener::Listener(BridgeSunset& bs, INetwork& nt, WorkDispatcher* disp, IScript::Request::Ref peventHandler, IScript::Request::Ref pcallback, IScript::Request::Ref pconnectCallback, const String& ip, bool h, bool mode) : network(nt), bridgeSunset(bs), listener(nullptr), dispatcher(disp), eventHandler(peventHandler), callback(pcallback), connectCallback(pconnectCallback) {
//	, http(h), activated(false), packetMode(mode)
	if (h) {
		Flag().fetch_or(LISTENER_HTTP, std::memory_order_relaxed);
	}

	if (mode) {
		Flag().fetch_or(LISTENER_PACKET_MODE, std::memory_order_relaxed);
	}

	listener = network.OpenListener(dispatcher->GetDispatcher(), Wrap(this, &Listener::OnEvent), Wrap(this, &Listener::OnAccept), ip);
	if (listener != nullptr && h) {
		network.SetListenerForHttpd(listener, Wrap(this, &Listener::OnAcceptHttp));
	}
}

bool Listener::IsValid() const {
	return listener != nullptr;
}

String Listener::GetAddress() {
	return network.GetAddress(listener);
}

void Listener::OnEvent(ITunnel::Listener*, INetwork::EVENT event) {
	bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(eventHandler, this, Looper::EventToString(event)));
}

Listener::~Listener() {
	if (listener != nullptr) {
		network.CloseListener(listener);
	}
}

const TWrapper<void, ITunnel::Connection*, INetwork::EVENT> Listener::OnAccept(INetwork::Connection* connection) {
	IScript::Request& r = bridgeSunset.GetScript().GetDefaultRequest();
	r.DoLock();
	IScript::Request::Ref ref = r.Reference(connectCallback);
	r.UnLock();

	WorkDispatcher* disp = dispatcher();
	INetwork::HttpRequest* req = (INetwork::HttpRequest*)nullptr;
	bool packetMode = !!(Flag().load(std::memory_order_acquire) & LISTENER_PACKET_MODE);
	TShared<Connection> c = TShared<Connection>::From(new Connection(bridgeSunset, network, disp, ref, "", connection, false, req, false, packetMode));

	IScript::BaseDelegate d(this);
	IScript::BaseDelegate dc(c());
	bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(callback, this, c));
	return Wrap(c(), &Connection::OnEvent);
}

void Listener::OnAcceptHttp(INetwork::Connection* connection, INetwork::HttpRequest* httpRequest) {
	IScript::Request& r = bridgeSunset.GetScript().GetDefaultRequest();
	r.DoLock();
	IScript::Request::Ref ref = r.Reference(connectCallback);
	r.UnLock();

	WorkDispatcher* disp = dispatcher();
	bool packetMode = !!(Flag().load(std::memory_order_acquire) & LISTENER_PACKET_MODE);
	TShared<Connection> c = TShared<Connection>::From(new Connection(bridgeSunset, network, disp, ref, "", connection, true, httpRequest, false, packetMode));

	bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(callback, this, c));
}

void Listener::ScriptUninitialize(IScript::Request& request) {
	if (callback) {
		request.Dereference(callback);
	}

	if (eventHandler) {
		request.Dereference(eventHandler);
	}

	if (connectCallback) {
		request.Dereference(connectCallback);
	}

	SharedTiny::ScriptUninitialize(request);
}

bool Listener::Activate() {
	bool ret = false;
	if (!(Flag().fetch_or(TINY_ACTIVATED, std::memory_order_acquire) & TINY_ACTIVATED)) {
		ret = network.ActivateListener(listener);
		Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release);
	}

	return ret;
}

void Listener::Deactivate() {
	if (Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release) & TINY_ACTIVATED) {
		network.DeactivateListener(listener);
	}
}
