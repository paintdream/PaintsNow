// INetwork.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-25
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "ITunnel.h"

namespace PaintsNow {
	class pure_interface INetwork : public ITunnel {
	public:
		~INetwork() override;
		class HttpRequest {};

		Dispatcher* OpenDispatcher() override = 0;
		void SetDispatcherOption(Dispatcher* dispatcher, const String& option, size_t value) override = 0;
		bool ActivateDispatcher(Dispatcher* dispatcher) override = 0;
		void DeactivateDispatcher(Dispatcher* dispatcher) override = 0;
		Dispatcher* GetListenerDispatcher(Listener* listener) override = 0;
		Dispatcher* GetConnectionDispatcher(Connection* connection) override = 0;
		void CloseDispatcher(Dispatcher* dispatcher) override = 0;
		std::vector<String> Resolve(const String& name) override = 0;

		// derived from ITunnel
		Listener* OpenListener(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Listener*, EVENT>& eventHandler, const TWrapper<const TWrapper<void, ITunnel::Connection*, EVENT>, Connection*>& callback, const String& address) override = 0;
		virtual void SetListenerForHttpd(Listener* listener, const TWrapper<void, Connection*, HttpRequest*>& httpRequestHandler) = 0;
		bool ActivateListener(Listener* listener) override = 0;
		String GetAddress(Listener* listener) override = 0;
		void DeactivateListener(Listener* listener) override = 0;
		void CloseListener(Listener* listener) override = 0;

		Connection* OpenConnection(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Connection*, EVENT>& callback, const String& address) override = 0;
		bool ActivateConnection(Connection* connection) override = 0;
		void GetConnectionAddresses(Connection* connection, String& from, String& to) override = 0;
		void Flush(Connection* connection) override = 0;
		bool ReadConnection(Connection* connection, void* data, size_t& length, size_t& mode) override = 0;
		bool WriteConnection(Connection* connection, const void* data, size_t length, size_t mode) override = 0;
		void DeactivateConnection(Connection* connection) override = 0;
		void CloseConnection(Connection* connection) override = 0;

		// URI, METHOD, CONTENT
		virtual void PrepareHttpRequest(HttpRequest* request) = 0;
		virtual String GetHttpRequestUri(HttpRequest* request) = 0;
		virtual void SetHttpRequestUri(HttpRequest* request, const String& uri) = 0;
		virtual String GetHttpRequestMethod(HttpRequest* request) = 0;
		virtual void SetHttpRequestMethod(HttpRequest* request, const String& method) = 0;
		virtual void GetHttpRequestHeader(HttpRequest* request, std::list<std::pair<String, String> >& header) = 0;
		virtual String GetHttpRequestData(HttpRequest* request) = 0;
		virtual void SetHttpRequestData(HttpRequest* request, const String& uri) = 0;
		virtual void SetHttpRequestHeader(HttpRequest* request, const std::list<std::pair<String, String> >& header) = 0;
		virtual void ParseUri(const String& uri, String& user, String& host, int& port, String& path, std::list<std::pair<String, String> >& query, String& fragment) = 0;
		virtual String MakeUri(const String& user, const String& host, const String& path, const std::list<std::pair<String, String> >& query, const String& fragment) = 0;

		virtual void MakeHttpRequest(HttpRequest* request) = 0;
		virtual void MakeHttpResponse(HttpRequest* request, int code, const String& reason, const String& data) = 0;

		virtual HttpRequest* OpenHttpRequest(Connection* connection, const TWrapper<void, int>& callback) = 0;
		virtual void CloseHttpRequest(HttpRequest* request) = 0;
	};
}

