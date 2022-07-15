// ZNetworkLibEvent.h
// PaintDream (paintdream@paintdream.com)
// 2015-10-24
//

#pragma once
#include "../../../Interface/INetwork.h"
#include "../../../../Core/Interface/IThread.h"

namespace PaintsNow {
	class ZNetworkLibEvent final : public INetwork {
	public:
		ZNetworkLibEvent(IThread& threadApi);
		~ZNetworkLibEvent() override;
		std::vector<String> Resolve(const String& name) override;
		Dispatcher* OpenDispatcher() override;
		void SetDispatcherOption(Dispatcher* dispatcher, const String& option, size_t value) override;
		bool ActivateDispatcher(Dispatcher* dispatcher) override;
		void DeactivateDispatcher(Dispatcher* dispatcher) override;
		Dispatcher* GetListenerDispatcher(Listener* listener) override;
		Dispatcher* GetConnectionDispatcher(Connection* connection) override;
		void CloseDispatcher(Dispatcher* dispatcher) override;

		Listener* OpenListener(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Listener*, EVENT>& eventHandler, const TWrapper<const TWrapper<void, ITunnel::Connection*, EVENT>, Connection*>& callback, const String& ip) override;
		void SetListenerForHttpd(Listener* listener, const TWrapper<void, Connection*, HttpRequest*>& httpRequestHandler) override;
		bool ActivateListener(Listener* listener) override;
		String GetAddress(Listener* listener) override;
		void DeactivateListener(Listener* listener) override;
		void CloseListener(Listener* listener) override;

		Connection* OpenConnection(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Connection*, EVENT>& connectCallback, const String& ip) override;
		bool ActivateConnection(Connection* connection) override;
		void DeactivateConnection(Connection* connection) override;
		bool ReadConnection(Connection* connection, void* data, size_t& length, size_t& mode) override;
		bool WriteConnection(Connection* connection, const void* data, size_t length, size_t mode) override;
		void Flush(Connection* connection) override;
		void GetConnectionAddresses(Connection* connection, String& src, String& dst) override;
		void CloseConnection(Connection* connection) override;

		// URI, METHOD, CONTENT
		void PrepareHttpRequest(HttpRequest* request) override;
		String GetHttpRequestUri(HttpRequest* request) override;
		void SetHttpRequestUri(HttpRequest* request, const String& uri) override;
		String GetHttpRequestMethod(HttpRequest* request) override;
		void SetHttpRequestMethod(HttpRequest* request, const String& method) override;
		void GetHttpRequestHeader(HttpRequest* request,  std::list<std::pair<String, String> >& header) override;
		void SetHttpRequestHeader(HttpRequest* request, const std::list<std::pair<String, String> >& header) override;

		String GetHttpRequestData(HttpRequest* request) override;
		void SetHttpRequestData(HttpRequest* request, const String& data) override;
		void ParseUri(const String& uri, String& user, String& host, int& port, String& path, std::list<std::pair<String, String> >& query, String& fragment) override;
		String MakeUri(const String& user, const String& host, const String& path, const std::list<std::pair<String, String> >& query, const String& fragment) override;

		void MakeHttpRequest(HttpRequest* request) override;
		void MakeHttpResponse(HttpRequest* request, int code, const String& reason, const String& data) override;

		HttpRequest* OpenHttpRequest(Connection* connection, const TWrapper<void, int>& callback) override;
		void CloseHttpRequest(HttpRequest* request) override;

	protected:
		IThread& threadApi;
	};
}
