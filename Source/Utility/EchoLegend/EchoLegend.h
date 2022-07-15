// EchoLegend.h -- multi-player client/server support
// PaintDream (paintdream@paintdream.com)
// 2015-5-29
// This project will be started at .... I don't know.
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/INetwork.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "Connection.h"
#include "Listener.h"
#include "WorkDispatcher.h"

namespace PaintsNow {
	class EchoLegend : public TReflected<EchoLegend, IScript::Library> {
	public:
		EchoLegend(IThread& threadApi, INetwork& network, INetwork& qnetwork, BridgeSunset& b);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void ScriptUninitialize(IScript::Request& request) override;

	public:
		/// <summary>
		/// Create a new Network Dispatcher, which schedules a set of listeners/connections on one thread. 
		/// </summary>
		/// <param name="quickNetwork"> Use quick network (KCP maybe)</param>
		/// <returns> Network Dispatcher object </returns>
		TShared<WorkDispatcher> RequestOpenDispatcher(IScript::Request& request, bool quickNetwork);

		/// <summary>
		/// Activate a network dispatcher.
		/// </summary>
		/// <param name="dispatcher"> Network Dispatcher object </param>
		/// <returns></returns>
		void RequestActivateDispatcher(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher);

		/// <summary>
		/// Deactive a network dispatcher
		/// </summary>
		/// <param name="dispatcher"> Network Dispatcher object </param>
		/// <returns></returns>
		void RequestDeactivateDispatcher(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher);

		/// <summary>
		/// Create listener on a dispatcher
		/// </summary>
		/// <param name="dispatcher"> Network Dispatcher object </param>
		/// <param name="address"> listen address, with format "xx.xx.xx.xx:port" </param>
		/// <param name="http"> whether it's a http listener </param>
		/// <param name="eventHandler"> listen event handler </param>
		/// <param name="callback"> network transmission callback handler </param>
		/// <param name="connectHandler"> connection (establish/close) callback handler </param>
		/// <param name="packetMode"> packet mode or stream mode </param>
		/// <returns> Listener object </returns>
		TShared<Listener> RequestOpenListener(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher, const String& address, bool http, IScript::Request::Ref eventHandler, IScript::Request::Ref callback, IScript::Request::Ref connectCallback, bool packetMode);

		/// <summary>
		/// Activate a listener
		/// </summary>
		/// <param name="listener"> Listener object </param>
		/// <returns></returns>
		void RequestActivateListener(IScript::Request& request, IScript::Delegate<Listener> listener);

		/// <summary>
		/// Get address string of listener
		/// </summary>
		/// <param name="listener"> Listener object </param>
		/// <returns></returns>
		String RequestGetListenerAddress(IScript::Request& request, IScript::Delegate<Listener> listener);

		/// <summary>
		/// Deactive a listener
		/// </summary>
		/// <param name="listener"> Listener object </param>
		/// <returns></returns>
		void RequestDeactivateListener(IScript::Request& request, IScript::Delegate<Listener> listener);

		/// <summary>
		/// Create a connection
		/// </summary>
		/// <param name="dispatcher"> Network Dispatcher object </param>
		/// <param name="address"> target address, with format "xx.xx.xx.xx:port" </param>
		/// <param name="http"> whether it's a http listener </param>
		/// <param name="connectHandler"> connection (establish/close) callback handler </param>
		/// <param name="packetMode"> packet mode or stream mode </param>
		/// <returns> Connection object </returns>
		TShared<Connection> RequestOpenConnection(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher, const String& address, bool http, IScript::Request::Ref connectHandler, bool packetMode);

		/// <summary>
		/// Activate a connection
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <returns></returns>
		void RequestActivateConnection(IScript::Request& request, IScript::Delegate<Connection> connection);

		/// <summary>
		/// Deactivate a connection
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <returns></returns>
		void RequestDeactivateConnection(IScript::Request& request, IScript::Delegate<Connection> listener);

		/// <summary>
		/// Get addresses string of connection (local, remote)
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <returns></returns>
		void RequestGetConnectionAddresses(IScript::Request& request, IScript::Delegate<Connection> connection);

		/// <summary>
		/// Write connection data
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <param name="data"> data string </param>
		/// <param name="mode"> packet mode, default to 0 </param>
		/// <returns></returns>
		void RequestWriteConnection(IScript::Request& request, IScript::Delegate<Connection> connection, StringView data, size_t mode);

		/// <summary>
		/// Read data from connection
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <returns> data read from connection </returns>
		String RequestReadConnection(IScript::Request& request, IScript::Delegate<Connection> connection);

		/// <summary>
		/// Write connection with http protocol request
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <param name="uri"> HTTP request location </param>
		/// <param name="method"> HTTP method </param>
		/// <param name="header"> HTTP optional header </param>
		/// <param name="data"> HTTP request data </param>
		void RequestWriteConnectionHttpRequest(IScript::Request& request, IScript::Delegate<Connection> connection, const String& uri, const String& method, std::list<std::pair<String, String> >& header, const String& data);

		/// <summary>
		/// Write connection with http protocol response
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <param name="code"> HTTP status </param>
		/// <param name="reason"> HTTP reponse reason </param>
		/// <param name="header"> HTTP reponse header </param>
		/// <param name="data"> HTTP reponse data </param>
		/// <returns></returns>
		void RequestWriteConnectionHttpResponse(IScript::Request& request, IScript::Delegate<Connection> connection, int code, const String& reason, std::list<std::pair<String, String> >& header, const String& data);

		/// <summary>
		/// Read connection and parse it with http request protocol
		/// </summary>
		/// <param name="connection"> Connection object </param>
		/// <returns> A dict with { "Uri" : string, "Method" : string, "Header" : { string : string }, "Data" : string } </returns>
		void RequestReadConnectionHttpRequest(IScript::Request& request, IScript::Delegate<Connection> connection);

		/// <summary>
		/// Parse a URI
		/// </summary>
		/// <param name="input"> URI string </param>
		/// <returns> A dict with { "User" : string, "Host" : string, "Path" : string, "Fragment" : string, "Query" : { string : string } } </returns>
		void RequestParseUri(IScript::Request& request, const String& input);

		/// <summary>
		/// Make a URI
		/// </summary>
		/// <param name="user"> user </param>
		/// <param name="host"> host address </param>
		/// <param name="path"> path </param>
		/// <param name="query"> query params </param>
		/// <param name="fragment"> fragment </param>
		/// <returns> URI string </returns>
		String RequestMakeUri(IScript::Request& request, const String& user, const String& host, const String& path, std::list<std::pair<String, String> >& query, const String& fragment);

	protected:
		INetwork& network;
		INetwork& networkQuick;
		BridgeSunset& bridgeSunset;
	};
}

