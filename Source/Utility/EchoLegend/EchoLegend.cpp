#include "EchoLegend.h"
#include "Listener.h"
#include "Connection.h"
#include <sstream>

using namespace PaintsNow;

EchoLegend::EchoLegend(IThread& threadApi, INetwork& n, INetwork& q, BridgeSunset& bs) : network(n), networkQuick(q), bridgeSunset(bs) {}

void EchoLegend::ScriptUninitialize(IScript::Request& request) {
	// TODO: Deactive all activive connections/listeners.
	Library::ScriptUninitialize(request);
}

TObject<IReflect>& EchoLegend::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestOpenListener)[ScriptMethod = "OpenListener"];
		ReflectMethod(RequestActivateListener)[ScriptMethod = "ActivateListener"];
		ReflectMethod(RequestGetListenerAddress)[ScriptMethodLocked = "GetListenerAddress"];
		ReflectMethod(RequestDeactivateListener)[ScriptMethod = "DeactivateListener"];
		ReflectMethod(RequestOpenConnection)[ScriptMethod = "OpenConnection"];
		ReflectMethod(RequestActivateConnection)[ScriptMethod = "ActivateConnection"];
		ReflectMethod(RequestDeactivateConnection)[ScriptMethod = "DeactivateConnection"];
		ReflectMethod(RequestGetConnectionAddresses)[ScriptMethodLocked = "GetConnectionAddresses"];
		ReflectMethod(RequestWriteConnection)[ScriptMethod = "WriteConnection"];
		ReflectMethod(RequestReadConnection)[ScriptMethod = "ReadConnection"];
		ReflectMethod(RequestWriteConnectionHttpRequest)[ScriptMethod = "WriteConnectionHttpRequest"];
		ReflectMethod(RequestWriteConnectionHttpResponse)[ScriptMethod = "WriteConnectionHttpResponse"];
		ReflectMethod(RequestReadConnectionHttpRequest)[ScriptMethod = "ReadConnectionHttpRequest"];
		ReflectMethod(RequestParseUri)[ScriptMethodLocked = "ParseUri"];
		ReflectMethod(RequestMakeUri)[ScriptMethodLocked = "MakeUri"];
		ReflectMethod(RequestOpenDispatcher)[ScriptMethod = "OpenDispatcher"];
		ReflectMethod(RequestActivateDispatcher)[ScriptMethod = "ActivateDispatcher"];
		ReflectMethod(RequestDeactivateDispatcher)[ScriptMethod = "DeactivateDispatcher"];
	}

	return *this;
}

TShared<Listener> EchoLegend::RequestOpenListener(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher, const String& ip, bool http, IScript::Request::Ref eventHandler, IScript::Request::Ref callback, IScript::Request::Ref connectCallback, bool packetMode) {
	CHECK_REFERENCES_WITH_TYPE(eventHandler U connectCallback U callback, IScript::Request::FUNCTION U IScript::Request::FUNCTION U IScript::Request::FUNCTION);
	CHECK_DELEGATE(dispatcher);
	CHECK_THREAD_IN_LIBRARY(dispatcher);

	WorkDispatcher* disp = dispatcher.Get();
	TShared<Listener> p = TShared<Listener>::From(new Listener(bridgeSunset, network, disp, eventHandler, callback, connectCallback, ip, http, packetMode));

	if (p->IsValid()) {
		p->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
		return p;
	} else {
		return nullptr;
	}
}

void EchoLegend::RequestActivateListener(IScript::Request& request, IScript::Delegate<Listener> listener) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(listener);
	CHECK_THREAD_IN_LIBRARY(listener);

	listener->Activate();
}

void EchoLegend::RequestDeactivateListener(IScript::Request& request, IScript::Delegate<Listener> listener) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(listener);
	listener->Deactivate();
}

String EchoLegend::RequestGetListenerAddress(IScript::Request& request, IScript::Delegate<Listener> listener) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(listener);

	return listener->GetAddress();
}

TShared<Connection> EchoLegend::RequestOpenConnection(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher, const String& ip, bool http, IScript::Request::Ref connectCallback, bool packetMode) {
	CHECK_REFERENCES_WITH_TYPE(connectCallback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(dispatcher);

	WorkDispatcher* disp = dispatcher.Get();
	ITunnel::Connection* conn = nullptr;
	INetwork::HttpRequest* req = nullptr;
	TShared<Connection> c = TShared<Connection>::From(new Connection(bridgeSunset, network, disp, connectCallback, ip, conn, http, req, false, packetMode));

	if (c->IsValid()) {
		c->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
		return c;
	} else {
		return nullptr;
	}
}

void EchoLegend::RequestActivateConnection(IScript::Request& request, IScript::Delegate<Connection> connection) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(connection);
	connection->Activate();
}

void EchoLegend::RequestDeactivateConnection(IScript::Request& request, IScript::Delegate<Connection> connection) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(connection);
	connection->Deactivate();
}

void EchoLegend::RequestGetConnectionAddresses(IScript::Request& request, IScript::Delegate<Connection> connection) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(connection);
	connection->GetAddress(request);
}

void EchoLegend::RequestWriteConnection(IScript::Request& request, IScript::Delegate<Connection> connection, StringView data, size_t mode) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(connection);
	connection->Write(data, mode);
}

String EchoLegend::RequestReadConnection(IScript::Request& request, IScript::Delegate<Connection> connection) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(connection);

	return connection->Read();
}

void EchoLegend::RequestWriteConnectionHttpRequest(IScript::Request& request, IScript::Delegate<Connection> connection, const String& uri, const String& method, std::list<std::pair<String, String> >& header, const String& data) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(connection);
	connection->WriteHttpRequest(uri, method, header, data);
}

void EchoLegend::RequestWriteConnectionHttpResponse(IScript::Request& request, IScript::Delegate<Connection> connection, int code, const String& reason, std::list<std::pair<String, String> >& header, const String& data) {
	if (connection) {
		connection->WriteHttpResponse(data, code, reason, header);
	} else {
		request.Error("EchoLegend::WriteConnectionHttpReponse(connection, data) : invalid connection");
	}
}

void EchoLegend::RequestReadConnectionHttpRequest(IScript::Request& request, IScript::Delegate<Connection> connection) {
	if (connection) {
		connection->ReadHttpRequest(request);
	}
}

void EchoLegend::RequestParseUri(IScript::Request& request, const String& url) {
	String user, host, path, fragment;
	int port;
	std::list<std::pair<String, String> > query;
	network.ParseUri(url, user, host, port, path, query, fragment);
	std::stringstream ss;
	ss << host << ":" << port;

	request << begintable
		<< key("User") << user
		<< key("Host") << StdToUtf8(ss.str())
		<< key("Path") << path
		<< key("Fragment") << fragment
		<< key("Query") << beginarray;

	for (std::list<std::pair<String, String> >::const_iterator p = query.begin(); p != query.end(); ++p) {
		request << key((*p).first.c_str()) << (*p).second;
	}

	request << endarray << endtable;
}

String EchoLegend::RequestMakeUri(IScript::Request& request, const String& user, const String& host, const String& path, std::list<std::pair<String, String> >& query, const String& fragment) {
	return network.MakeUri(user, host, path, query, fragment);
}

TShared<WorkDispatcher> EchoLegend::RequestOpenDispatcher(IScript::Request& request, bool useQuickNetwork) {
	ITunnel::Dispatcher* disp = useQuickNetwork ? networkQuick.OpenDispatcher() : network.OpenDispatcher();
	if (disp != nullptr) {
		TShared<WorkDispatcher> dispatcher = TShared<WorkDispatcher>::From(new WorkDispatcher(bridgeSunset, useQuickNetwork ? networkQuick : network, disp));
		dispatcher->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
		return dispatcher;
	} else {
		return nullptr;
	}
}

void EchoLegend::RequestActivateDispatcher(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(dispatcher);

	dispatcher->AsyncActivate(request);
}

void EchoLegend::RequestDeactivateDispatcher(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(dispatcher);

	dispatcher->Deactivate();
}