// RemoteCall.h
// PaintDream (paintdream@paintdream.com)
// 2016-7-15
//

#pragma once
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../General/Interface/ITunnel.h"
#include "../../Core/System/Tiny.h"
#include "../../Core/System/MemoryStream.h"
#include "../../Core/Template/TQueue.h"

namespace PaintsNow {
	class RemoteCall {
	public:
		enum STATUS { CONNECTED = 0, CLOSED, ABORTED, TIMEOUT };
		class Context : public SharedTiny {};

	protected:
		class RequestBase : public SharedTiny {
		public:
			virtual void Prepare(IStreamBase& stream) = 0;
			virtual void Complete(RemoteCall&, IStreamBase& stream) = 0;

			String name;
		};

		template <class Input, class Output>
		class Request : public TReflected<Request<Input, Output>, RequestBase> {
		public:
			Request(const String& n, rvalue<Input> input, const TWrapper<void, RemoteCall&, rvalue<Output> >& callback) : wrapper(callback),
#if defined(_MSC_VER) && _MSC_VER <= 1200
				inputPacket(input)
#else
				inputPacket(std::move(input))
#endif
			{
				RequestBase::name = n;
			}

			void Prepare(IStreamBase& stream) override {
				stream << inputPacket;
			}

			void Complete(RemoteCall& remoteCall, IStreamBase& stream) override {
				Output outputPacket;
				stream >> outputPacket;

				wrapper(remoteCall, std::move(outputPacket));
			}

			Input inputPacket;
			TWrapper<void, RemoteCall&, rvalue<Output> > wrapper;
		};

		class ResponseBase : public SharedTiny {
		public:
			virtual bool Handle(RemoteCall& remoteCall, IStreamBase& outputStream, IStreamBase& inputStream, const TShared<Context>& context, uint32_t id) = 0;
		};

		template <class Input, class Output>
		class Response : public TReflected<Response<Input, Output>, ResponseBase> {
		public:
			Response(const TWrapper<bool, RemoteCall&, Output&, Input&, const TShared<Context>&, uint32_t>& handler) : wrapper(handler) {}

			bool Handle(RemoteCall& remoteCall, IStreamBase& outputStream, IStreamBase& inputStream, const TShared<Context>& context, uint32_t id) override {
				inputStream >> inputPacket;
				bool sync = wrapper(remoteCall, outputPacket, inputPacket, context, id);
				if (sync) {
					outputStream << std::move(outputPacket);
				}

				return sync;
			}

			TWrapper<bool, RemoteCall&, Output&, Input&, const TShared<Context>&, uint32_t> wrapper;
			Input inputPacket;
			Output outputPacket;
		};

		class Session : public TReflected<Session, Context> {
		public:
			Session(RemoteCall& remoteCall);
			~Session() override;
			void HandleEvent(ITunnel::EVENT event);
			void Flush();
			void Process();

			template <class Output>
			void Complete(uint32_t id, Output& outputPacket, MemoryStream& serializeStream) {
				IThread& thread = remoteCall.GetThreadApi();
				ITunnel& tunnel = remoteCall.GetTunnel();
				serializeStream << String("") << id;

				IFilterBase& filter = remoteCall.GetFilter();
				IStreamBase* outputEncodeStream = filter.CreateFilter(serializeStream);
				*outputEncodeStream << outputPacket;
				outputEncodeStream->Destroy();

				ITunnel::Packet packet;
				packet.header.length = verify_cast<ITunnel::PacketSizeType>(serializeStream.GetOffset());
				thread.DoLock(sessionLock);
				tunnel.WriteConnectionPacket(connection, serializeStream.GetBuffer(), verify_cast<ITunnel::PacketSizeType>(serializeStream.GetOffset()), packet);
				thread.UnLock(sessionLock);
			}

			IThread::Lock* sessionLock;
			TQueueList<TShared<RequestBase> > requestQueue;
			TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS> clientStatusHandler;
			std::unordered_map<uint32_t, TShared<RequestBase> > completionMap;
			MemoryStream inputStream;
			MemoryStream outputStream;
			ITunnel::Packet currentState;
			RemoteCall& remoteCall;
			ITunnel::Connection* connection;
			uint32_t requestID;
		};

	public:	
		RemoteCall(IThread& threadApi, ITunnel& tunnel, IFilterBase& filter, const String& entry = "", const TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS>& serverStatusHandler = nullptr);
		~RemoteCall();

		bool Start();
		void Reset();
		void Stop();
		void Connect(const TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS>& clientStatusHandler, const String& target);

		ITunnel& GetTunnel() { return tunnel; }
		IFilterBase& GetFilter() { return filter; }

		// Compatible for VC6
#if defined(_MSC_VER) && _MSC_VER <= 1200
		template <class F>
		void Register(const String& name, const F& wrapper) {
			requestHandlers[name] = TShared<ResponseBase>::From(new Response<std::remove_reference<F::_C>::type, std::remove_reference<F::_B>::type>(wrapper));
		}
#else
		template <class Input, class Output>
		void Register(const String& name, const TWrapper<bool, RemoteCall&, Output&, Input&, const TShared<Context>&, uint32_t>& wrapper) {
			requestHandlers[name] = TShared<ResponseBase>::From(new Response<Input, Output>(wrapper));
		}
#endif

		template <class Input, class Output>
		void Call(const String& name, rvalue<Input> input, const TWrapper<void, RemoteCall&, rvalue<Output> >& callback) {
			Session* session = outputSession();
			assert(session != nullptr);
			session->requestQueue.Push(TShared<RequestBase>::From(new Request<Input, Output>(name,
#if defined(_MSC_VER) && _MSC_VER <= 1200
				input,
#else
				std::move(input),
#endif
			callback)));
		}

		template <class Output>
		void Complete(const TShared<Context>& context, uint32_t responseID, Output& outputPacket, MemoryStream& serializeStream) {
			Session* session = static_cast<Session*>(context());
			session->Complete(responseID, outputPacket, serializeStream);
		}

		void Flush();

	protected:
		const ITunnel::Handler OnConnection(ITunnel::Connection* connection);
		bool ThreadProc(IThread::Thread* thread, size_t context);
		void HandleEvent(ITunnel::EVENT event);

	public:
		const std::unordered_map<String, TShared<ResponseBase> >& GetRequestHandlers() const { return requestHandlers; }
		const TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS>& GetServerStatusHandler() const { return serverStatusHandler; }
		IThread& GetThreadApi() { return threadApi; }
		
	protected:
		IThread& threadApi;
		ITunnel& tunnel;
		IFilterBase& filter;

		String entry;
		ITunnel::Dispatcher* dispatcher;
		std::atomic<IThread::Thread*> dispThread;
		TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS> serverStatusHandler;
		std::unordered_map<String, TShared<ResponseBase> > requestHandlers;
		std::vector<TShared<Session> > inputSessions;
		TShared<Session> outputSession;
		size_t currentResponseID;
	};
}
