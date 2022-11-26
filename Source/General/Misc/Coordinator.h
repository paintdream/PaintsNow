// Coordinator.h
// PaintDream (paintdream@paintdream.com)
// 2022-11-5
//

#pragma once

#include "RemoteCall.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	class ProtoInputLogin : public TReflected<ProtoInputLogin, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(clientVersion);
				ReflectProperty(platform);
				ReflectProperty(listenAddress);
				ReflectProperty(coreCount);
				ReflectProperty(totalFreqMhz);
			}

			return *this;
		}

		String clientVersion;
		String platform;
		String listenAddress;
		uint32_t coreCount;
		uint32_t totalFreqMhz;
	};

	class ProtoOutputLogin : public TReflected<ProtoOutputLogin, ProtoOutput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(serverVersion);
				ReflectProperty(supportProtocols);
			}

			return *this;
		}

		String serverVersion;
		std::vector<String> supportProtocols;
	};

	class ProtoInputLogout : public TReflected<ProtoInputLogout, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(clientVersion);
				ReflectProperty(reason);
			}

			return *this;
		}

		String clientVersion;
		String reason;
	};

	class ProtoOutputLogout : public TReflected<ProtoOutputLogout, ProtoOutput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(serverVersion);
			}

			return *this;
		}

		String serverVersion;
	};

	class ProtoInputPush : public TReflected<ProtoInputPush, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(protocol);
				ReflectProperty(cookie);
			}

			return *this;
		}

		String protocol;
		String cookie;
	};

	class ProtoOutputPush : public TReflected<ProtoOutputPush, ProtoOutput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(status);
			}

			return *this;
		}

		String status; // "" for accept
	};

	class ProtoInputPull : public TReflected<ProtoInputPull, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(cookie);
			}

			return *this;
		}

		String cookie;
	};

	class ProtoOutputPull : public TReflected<ProtoOutputPull, ProtoOutput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(cookie);
				ReflectProperty(payload);
			}

			return *this;
		}

		String cookie;
		String payload;
	};

	class ProtoInputVerbose : public TReflected<ProtoInputVerbose, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(cookie);
				ReflectProperty(logText);
			}

			return *this;
		}

		String cookie;
		String logText;
	};

	class ProtoOutputVerbose : public TReflected<ProtoOutputVerbose, ProtoOutput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {}

			return *this;
		}
	};

	class ProtoInputComplete : public TReflected<ProtoInputComplete, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(cookie);
			}

			return *this;
		}

		String cookie;
		String status; // "" for complete
	};

	class ProtoOutputComplete : public TReflected<ProtoOutputComplete, ProtoOutput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(cookie);
			}

			return *this;
		}

		String cookie;
	};

	class Coordinator : public WarpTiny, public RemoteCall {
	public:
		Coordinator(IThread& threadApi, ITunnel& tunnel, IFilterBase& filter, const String& entry = "");
		
		class Session : public TReflected<Session, RemoteCall::Session> {
		public:
			Session(RemoteCall& remoteCall);
			const String& GetListenAddress() const;
			void SetListenAddress(const String& address);

		protected:
			void HandleEvent(ITunnel::Connection* connection, ITunnel::EVENT status);
			String listenAddress;
		};

		struct RequestInfo {
			RequestInfo() {}
			RequestInfo(rvalue<RequestInfo> info) : cookie(std::move(((RequestInfo&)info).cookie)), payload(std::move(((RequestInfo&)info).payload)), upstream(std::move(((RequestInfo&)info).upstream)), status(std::move(((RequestInfo&)info).status)) {}

			RequestInfo& operator = (rvalue<RequestInfo> rhs) {
				RequestInfo& info = rhs;
				cookie = std::move(info.cookie);
				payload = std::move(info.payload);
				upstream = std::move(info.upstream);
				status = std::move(info.status);

				return *this;
			}

			String cookie;
			String payload;
			String upstream;
			String status;
		};

		void PushRequest(rvalue<RequestInfo> requestInfo);
		bool PullRequest(RequestInfo& requestInfo);
		void CompleteRequest(rvalue<RequestInfo> requestInfo);

		void AddUpstream(const String& listenAddress);
		void RemoveUpstream(const String& listenAddress);
		void AddDownstream(const String& listenAddress);
		void RemoveDownstream(const String& listenAddress);

		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		TShared<RemoteCall::Session> CreateSession() override;
		void AddActiveSession(const TShared<Session>& session);
		void RemoveActiveSession(const TShared<Session>& session);
		void RemoveSession(const TShared<RemoteCall::Session>& session) override;

	protected:
		bool RpcLogin(RemoteCall& remoteCall, ProtoOutputLogin& outputPacket, rvalue<ProtoInputLogin> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcLogout(RemoteCall& remoteCall, ProtoOutputLogout& outputPacket, rvalue<ProtoInputLogout> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPush(RemoteCall& remoteCall, ProtoOutputPush& outputPacket, rvalue<ProtoInputPush> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPull(RemoteCall& remoteCall, ProtoOutputPull& outputPacket, rvalue<ProtoInputPull> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcVerbose(RemoteCall& remoteCall, ProtoOutputVerbose& outputPacket, rvalue<ProtoInputVerbose> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcComplete(RemoteCall& remoteCall, ProtoOutputComplete& outputPacket, rvalue<ProtoInputComplete> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);

	protected:
		void RetPull(RemoteCall& remoteCall, rvalue<ProtoOutputPull> outputPacket);

	protected:
		TQueueList<RequestInfo> pendingRequestInfos;
		TQueueList<RequestInfo> completedRequestInfos;
		std::vector<String> upstreams;
		std::vector<String> downstreams;
		std::unordered_map<String, TShared<Session> > activeSessions;
		std::unordered_map<String, RequestInfo*> queryRequestMap;
	};
}
