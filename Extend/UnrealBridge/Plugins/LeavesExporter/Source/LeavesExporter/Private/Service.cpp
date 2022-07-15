#include "LeavesExporterPCH.h"
#include "Service.h"
#include "../../../../../Source/General/Driver/Network/LibEvent/ZNetworkLibEvent.h"
#include "../../../../../Source/Core/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../../../../Source/Core/Driver/Filter/Pod/ZFilterPod.h"
#include "../../../../../Source/Core/System/ThreadPool.h"
#include "../../../../../Source/Core/System/Kernel.h"
#include "../../../../../Source/Core/System/ConsoleStream.h"

namespace PaintsNow {
	class DummyUniformResourceManager : public IUniformResourceManager {
	public:
		virtual TShared<ResourceBase> CreateResource(const String& location, const String& extension = "", bool openExisting = true, Tiny::FLAG flag = 0) override {
			return nullptr;
		}

		virtual bool SaveResource(const TShared<ResourceBase>& resource, const String& extension = "") override {
			return false;
		}
		virtual bool LoadResource(const TShared<ResourceBase>& resource, const String& extension = "") override {
			return false;
		}
	};

	static ZThreadPthread uniqueThreadApi;
	static ThreadPool threadPool(uniqueThreadApi, 0);
	static Kernel kernel(threadPool, 2);
	static ZNetworkLibEvent libEvent(uniqueThreadApi);
	static ConsoleStream logStream(stdout);
	static ZFilterPod assetFilter;
	static DummyUniformResourceManager uniformResourceManager;

	class UnrealResourceManager : public ResourceManager {
	public:
		UnrealResourceManager(Kernel& kernel, IUniformResourceManager& hostManager) : ResourceManager(kernel, hostManager, logStream, logStream, nullptr) {}
		virtual Unique GetDeviceUnique() const {
			return UniqueType<UObject>::Get();
		}

		virtual void InvokeRefresh(ResourceBase* resource, void* deviceContext) override {}
		virtual void InvokeAttach(ResourceBase* resource, void* deviceContext) override {}
		virtual void InvokeDetach(ResourceBase* resource, void* deviceContext) override {}
		virtual void InvokeUpload(ResourceBase* resource, void* deviceContext) override {}
		virtual void InvokeDownload(ResourceBase* resource, void* deviceContext) override {}
	};

	void Service::StatusHandler(RemoteCall& remoteCall, ITunnel::Connection* connection, RemoteCall::STATUS status) {
		if (status == RemoteCall::CONNECTED) {
			ProtoInputCheckVersion inputPacket;
			remoteCall.Call("RpcCheckVersion", std::move(inputPacket), Wrap(this, &Service::OnCheckVersion));
			remoteCall.Flush();
		}
	}

	void Service::Initialize(ISceneExplorer* se) {
		sceneExp = se;
		resourceManager = std::make_unique<UnrealResourceManager>(kernel, uniformResourceManager);
		remoteCall = std::make_unique<RemoteCall>(uniqueThreadApi, libEvent, assetFilter, "", Wrap(this, &Service::StatusHandler));
		remoteCall->Start();
	}

	void Service::Reconnect(const String& port) {
		remoteCall->Connect(Wrap(this, &Service::StatusHandler), port);
	}

	void Service::Uninitialize() {
		// order assurance 
		remoteCall.release();
	}

	void Service::OnCheckVersion(RemoteCall& remoteCall, ProtoOutputCheckVersion&& outputPacket) {
		clientVersion = outputPacket.clientVersion;

		String str = "OnCheckVersion(): ";
		str += clientVersion;
		sceneExp->WriteLog(ISceneExplorer::LOG_TEXT, str.c_str());
	}

	ResourceManager& Service::GetResourceManager() const {
		assert(resourceManager.get() != nullptr);
		return *resourceManager.get();
	}

	bool Service::IsConnected() const {
		return !clientVersion.empty();
	}
}