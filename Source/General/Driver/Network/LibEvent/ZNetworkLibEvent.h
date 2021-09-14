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
		void EnumerateIPAddresses(const TWrapper<void, const String&>& callback) override;
		Dispatcher* OpenDispatcher() override;
		bool ActivateDispatcher(Dispatcher* dispatcher) override;
		void DeactivateDispatcher(Dispatcher* dispatcher) override;
		Dispatcher* GetListenerDispatcher(Listener* listener) override;
		Dispatcher* GetConnectionDispatcher(Connection* connection) override;
		void CloseDispatcher(Dispatcher* dispatcher) override;

		Listener* OpenListener(Dispatcher* dispatcher, const TWrapper<void, EVENT>& eventHandler, const TWrapper<const TWrapper<void, EVENT>, Connection*>& callback, const String& ip) override;
		bool ActivateListener(Listener* listener) override;
		void GetListenerInfo(Listener* listener, String& ip) override;
		void DeactivateListener(Listener* listener) override;
		void CloseListener(Listener* listener) override;

		Connection* OpenConnection(Dispatcher* dispatcher, const TWrapper<void, EVENT>& connectCallback, const String& ip) override;
		bool ActivateConnection(Connection* connection) override;
		void DeactivateConnection(Connection* connection) override;
		bool ReadConnection(Connection* connection, void* data, size_t& length) override;
		bool WriteConnection(Connection* connection, const void* data, size_t& length) override;
		void Flush(Connection* connection) override;
		void GetConnectionInfo(Connection* connection, String& src, String& dst) override;
		void CloseConnection(Connection* connection) override;

		Httpd* OpenHttpd(Listener* listener, const TWrapper<void, Connection*, HttpRequest*>& requestHandler) override;
		bool ActivateHttpd(Httpd* httpd) override;
		void CloseHttpd(Httpd* httpd) override;

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
		virtual bool ActivateListenerWithHttpd(Listener* listener, Httpd* httpd);

	protected:
		IThread& threadApi;
	};
}
