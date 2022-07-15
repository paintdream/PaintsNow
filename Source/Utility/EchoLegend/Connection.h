// Connection.h
// PaintDream (paintdream@paintdream.com)
// 2015-6-27
//

#pragma once
#include "WorkDispatcher.h"
#include "../../Core/System/MemoryStream.h"

namespace PaintsNow {
	class Connection : public TReflected<Connection, WarpTiny> {
	public:
		Connection(BridgeSunset& bridgeSunset, INetwork& network, WorkDispatcher* dispatcher, IScript::Request::Ref connectCallback, const String& ip, INetwork::Connection* connection, bool http, INetwork::HttpRequest* httpRequest, bool ownReq, bool packetMode);
		enum {
			CONNECTION_HTTP = WARP_CUSTOM_BEGIN,
			CONNECTION_OWN_CONNECTION = WARP_CUSTOM_BEGIN << 1,
			CONNECTION_OWN_REQUEST = WARP_CUSTOM_BEGIN << 2,
			CONNECTION_PACKET_MODE = WARP_CUSTOM_BEGIN << 3,
			CONNECTION_CUSTOM_BEGIN = WARP_CUSTOM_BEGIN << 4
		};

		~Connection() override;
		bool IsValid() const;

		virtual bool Activate();
		virtual void Deactivate();

		void OnEvent(ITunnel::Connection*, INetwork::EVENT event);
		void OnEventHttp(int code);
		void ScriptUninitialize(IScript::Request& request) override;
		String Read();
		void Write(StringView data, size_t mode);

		void GetAddress(IScript::Request& request);
		void ReadHttpRequest(IScript::Request& request);

		void WriteHttpRequest(const String& uri, const String& method, const std::list<std::pair<String, String> >& header, const String& data);
		void WriteHttpResponse(const String& data, int code, const String& reason, std::list<std::pair<String, String> >& header);
		IScript::Request::Ref GetCallback() const;

	protected:
		void DispatchEvent(INetwork::EVENT event);

	protected:
		INetwork& network;
		BridgeSunset& bridgeSunset;
		WorkDispatcher* dispatcher;
		INetwork::Connection* connection;
		INetwork::HttpRequest* httpRequest;
		IScript::Request::Ref callback;
		String currentData;
	};
}

