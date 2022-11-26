// Service.h
// By PaintDream (paintdream@paintdream.com)
// 2018-2-9
//

#pragma once

#include "../../../../../Source/Utility/GalaxyWeaver/Protocol.h"
#include "../../../../../Source/General/Misc/RemoteCall.h"
#include "ISceneExplorer.h"
#include <memory>

namespace PaintsNow {
	class ResourceManager;
	class Service : public RemoteCall {
	public:
		Service();
		void Initialize(ISceneExplorer* sceneExp);
		void Uninitialize();

		void Reconnect(const String& port);
		operator bool() const {
			return IsConnected();
		}

		ResourceManager& GetResourceManager() const;
		bool IsConnected() const;
		RemoteCall::Session& GetSession() { return *session; }

		class Session : public TReflected<Session, RemoteCall::Session> {
		public:
			Session(RemoteCall& remoteCall) : BaseClass(remoteCall) {}
			void HandleEvent(ITunnel::Connection* connection, ITunnel::EVENT status) override;
		};

	protected:
		void OnCheckVersion(RemoteCall& remoteCall, ProtoOutputCheckVersion&& outputPacket);

	protected:
		TShared<RemoteCall::Session> CreateSession();

	protected:
		std::unique_ptr<RemoteCall> remoteCall;
		std::unique_ptr<ResourceManager> resourceManager;
		TShared<RemoteCall::Session> session;
		String clientVersion;
		ISceneExplorer* sceneExp;
	};
}

