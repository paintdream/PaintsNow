#include "Connection.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

Connection::Connection(BridgeSunset& bs, INetwork& nt, WorkDispatcher* disp, IScript::Request::Ref cb, const String& ip, INetwork::Connection* con, bool http, INetwork::HttpRequest* httpReq, bool ownReq, bool mode) : bridgeSunset(bs), network(nt),  dispatcher(disp), connection(con), httpRequest(httpReq), callback(cb) {
	uint32_t bitMask = 0;
	if (ownReq) {
		bitMask |= CONNECTION_OWN_REQUEST;
	}

	if (mode) {
		bitMask |= CONNECTION_PACKET_MODE;
	}

	if (http) {
		bitMask |= CONNECTION_HTTP;
	}

	if (connection == nullptr) {
		connection = network.OpenConnection(dispatcher->GetDispatcher(), Wrap(this, &Connection::OnEvent), ip);
		bitMask |= CONNECTION_OWN_CONNECTION;
	} else {
		bitMask |= TINY_ACTIVATED;
	}

	Flag().fetch_or(bitMask, std::memory_order_relaxed);

	if (connection != nullptr) {
		dispatcher->ReferenceObject();

		if (Flag().load(std::memory_order_relaxed) & CONNECTION_HTTP) {
			if (httpRequest == nullptr) {
				httpRequest = network.OpenHttpRequest(connection, Wrap(this, &Connection::OnEventHttp));
				Flag().fetch_or(CONNECTION_OWN_REQUEST, std::memory_order_relaxed);
			}
		}
	}
}

bool Connection::IsValid() const {
	return connection != nullptr && (!(Flag().load(std::memory_order_acquire) & CONNECTION_HTTP) || httpRequest != nullptr);
}

void Connection::Deactivate() {
	if (Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed) & TINY_ACTIVATED) {
		network.DeactivateConnection(connection);
	}
}

Connection::~Connection() {
	if (httpRequest != nullptr && (Flag().load(std::memory_order_acquire) & CONNECTION_OWN_REQUEST)) {
		network.CloseHttpRequest(httpRequest);
	}

	if (connection != nullptr && (Flag().load(std::memory_order_acquire) & CONNECTION_OWN_CONNECTION)) {
		network.CloseConnection(connection);
	}

	if (dispatcher != nullptr) {
		dispatcher->ReleaseObject();
	}
}

bool Connection::Activate() {
	bool ret = false;
	if (!(Flag().fetch_or(TINY_ACTIVATED, std::memory_order_acquire) & TINY_ACTIVATED)) {
		ret = network.ActivateConnection(connection);
	}

	return ret;
}

IScript::Request::Ref Connection::GetCallback() const {
	return callback;
}

void Connection::OnEventHttp(int code) {
	OPTICK_EVENT();

	IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
	req.DoLock();
	req.Push();
	req.Call(callback, this, code);
	req.Pop();
	req.UnLock();
	bridgeSunset.requestPool.ReleaseSafe(&req);
}

void Connection::OnEvent(ITunnel::Connection* c, INetwork::EVENT event) {
	OPTICK_EVENT();
	if (event == INetwork::READ && (Flag().load(std::memory_order_acquire) & CONNECTION_PACKET_MODE)) {
		size_t size = 0;
		size_t mode = 0; // PACKETED

		if (network.ReadConnection(connection, nullptr, size, mode)) {
			if (currentData.size() < size) {
				currentData.resize(size);
			}

			mode = 0;
			if (network.ReadConnection(connection, const_cast<char*>(currentData.data()), size, mode)) {
				DispatchEvent(event);
			}
		}
	} else {
		DispatchEvent(event);

		if ((Flag().load(std::memory_order_acquire) & CONNECTION_HTTP) && event == INetwork::CONNECTED) {
			OnEvent(c, INetwork::READ);
		}
	}
}

void Connection::DispatchEvent(INetwork::EVENT event) {
	OPTICK_EVENT();
	IScript::Request& req = *bridgeSunset.requestPool.AcquireSafe();
	req.DoLock();
	req.Push();
	if (event == INetwork::READ && ((Flag().load(std::memory_order_relaxed) & CONNECTION_PACKET_MODE))) {
		req.Call(callback, this, Looper::EventToString(event), currentData);
	} else {
		req.Call(callback, this, Looper::EventToString(event));
	}
	req.Pop();
	req.UnLock();
	bridgeSunset.requestPool.ReleaseSafe(&req);
}

void Connection::ScriptUninitialize(IScript::Request& request) {
	if (callback) {
		request.Dereference(callback);
	}

	SharedTiny::ScriptUninitialize(request);
}

String Connection::Read() {
	OPTICK_EVENT();
	if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
		size_t packetMode = (Flag().load(std::memory_order_acquire) & CONNECTION_PACKET_MODE) ? 0 : ITunnel::STREAMED;
		size_t length = 0;
		size_t mode = 0;
		if (!network.ReadConnection(connection, nullptr, length, mode)) {
			return "";
		}

		String data;
		data.resize(length);
		network.ReadConnection(connection, const_cast<char*>(data.data()), length, mode);
		data.resize(length);

		return data;
	} else {
		return "";
	}
}

void Connection::Write(StringView data, size_t mode) {
	OPTICK_EVENT();
	if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
		network.WriteConnection(connection, data.data(), data.length(), mode);
		network.Flush(connection);
	}
}

void Connection::GetAddress(IScript::Request& request) {
	String src, dst;
	network.GetConnectionAddresses(connection, src, dst);
	assert(!src.empty());

	request << begintable
		<< key("Source") << src
		<< key("Destination") << dst
		<< endtable;
}

void Connection::ReadHttpRequest(IScript::Request& request) {
	if (httpRequest != nullptr) {
		std::list<std::pair<String, String> > header;
		network.GetHttpRequestHeader(httpRequest, header);
		String uri = network.GetHttpRequestUri(httpRequest);
		String method = network.GetHttpRequestMethod(httpRequest);
		String data = network.GetHttpRequestData(httpRequest);

		request.DoLock();
		request << begintable
			<< key("Uri") << uri
			<< key("Method") << method
			<< key("Header") << header
			<< key("Data") << data
			<< endtable;
		request.UnLock();
	}
}

void Connection::WriteHttpRequest(const String& uri, const String& method, const std::list<std::pair<String, String> >& header, const String& data) {
	if (httpRequest != nullptr) {
		network.PrepareHttpRequest(httpRequest);
		network.SetHttpRequestMethod(httpRequest, method);
		network.SetHttpRequestUri(httpRequest, uri);
		network.SetHttpRequestHeader(httpRequest, header);
		network.SetHttpRequestData(httpRequest, data);
		network.MakeHttpRequest(httpRequest);
	}
}

void Connection::WriteHttpResponse(const String& data, int code, const String& reason, std::list<std::pair<String, String> >& header) {
	if (httpRequest != nullptr) {
		network.SetHttpRequestHeader(httpRequest, header);
		network.MakeHttpResponse(httpRequest, code, reason, data);
	}
}

