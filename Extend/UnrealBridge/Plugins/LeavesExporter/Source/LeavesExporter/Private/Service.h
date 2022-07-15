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
	class Service {
	public:
		void Initialize(ISceneExplorer* sceneExp);
		void Uninitialize();

		void Reconnect(const String& port);
		operator bool() const {
			return IsConnected();
		}

		RemoteCall* operator -> () const {
			return remoteCall.get();
		}

		ResourceManager& GetResourceManager() const;
		bool IsConnected() const;

	protected:
		void StatusHandler(RemoteCall& remoteCall, ITunnel::Connection* connection, RemoteCall::STATUS status);
		void OnCheckVersion(RemoteCall& remoteCall, ProtoOutputCheckVersion&& outputPacket);

	protected:
		std::unique_ptr<RemoteCall> remoteCall;
		std::unique_ptr<ResourceManager> resourceManager;
		String clientVersion;
		ISceneExplorer* sceneExp;
	};
}

