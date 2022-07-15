// Listener.h
// PaintDream (paintdream@paintdream.com)
// 2015-6-27
//

#pragma once
#include "WorkDispatcher.h"

namespace PaintsNow {
	class Listener : public TReflected<Listener, WarpTiny> {
	public:
		Listener(BridgeSunset& bridgeSunset, INetwork& network, WorkDispatcher* disp, IScript::Request::Ref eventHandler, IScript::Request::Ref callback, IScript::Request::Ref connectCallback, const String& ip, bool http, bool packetMode);
		~Listener() override;
		enum {
			LISTENER_HTTP = WARP_CUSTOM_BEGIN,
			LISTENER_PACKET_MODE = WARP_CUSTOM_BEGIN << 1,
			LISTENER_CUSTOM_BEGIN = WARP_CUSTOM_BEGIN << 2
		};

		bool IsValid() const;

		void OnEvent(ITunnel::Listener*, INetwork::EVENT event);
		const TWrapper<void, ITunnel::Connection*, INetwork::EVENT> OnAccept(INetwork::Connection* connection);
		void OnAcceptHttp(INetwork::Connection* connection, INetwork::HttpRequest* request);
		void ScriptUninitialize(IScript::Request& request) override;
		String GetAddress();
		virtual bool Activate();
		virtual void Deactivate();

	protected:
		BridgeSunset& bridgeSunset;
		INetwork& network;
		TShared<WorkDispatcher> dispatcher;
		INetwork::Listener* listener;
		IScript::Request::Ref eventHandler;
		IScript::Request::Ref callback;
		IScript::Request::Ref connectCallback;
	};
}

