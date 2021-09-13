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
	protected:
		class RequestBase : public SharedTiny {
		public:
			RequestBase(const String& n) : name(n) {}
			virtual void Prepare(IStreamBase& stream) = 0;
			virtual void Complete(IStreamBase& stream) = 0;

			String name;
		};

		template <class Input, class Output>
		class Request : public TReflected<Request<Input, Output>, RequestBase> {
			Request(const String& n, rvalue<Input> input, const TWrapper<void, rvalue<Output> >& callback) : RequestBase(n), wrapper(callback),
#if defined(_MSC_VER) && _MSC_VER <= 1200
				inputPacket(input)
#else
				inputPacket(std::move(input))
#endif
			{}

			void Prepare(IStreamBase& stream) override {
				stream << inputPacket;
			}

			void Complete(IStreamBase& stream) override {
				Output outputPacket;
				stream >> outputPacket;

				wrapper(std::move(outputPacket));
			}

			Input inputPacket;
			TWrapper<void, rvalue<Output> > wrapper;
		};

		class ResponseBase : public SharedTiny {
		public:
			virtual void Handle(IStreamBase& outputStream, IStreamBase& inputStream) = 0;
		};

		template <class Input, class Output>
		class Response : public TReflected<Response<Input, Output>, ResponseBase> {
		public:
			Response(const TWrapper<void, Output&, rvalue<Input> >& handler) : wrapper(handler) {}

			void Handle(IStreamBase& outputStream, IStreamBase& inputStream) override {
				Input inputPacket;
				Output outputPacket;

				inputStream >> inputPacket;
				wrapper(outputPacket, std::move(inputPacket));
				outputStream << std::move(outputPacket);
			}

			TWrapper<void, Output&, rvalue<Input> > wrapper;
		};

		class Session : public TReflected<Session, SharedTiny> {
		public:
			Session(RemoteCall& remoteCall);
			~Session() override;
			void HandleEvent(ITunnel::EVENT event);
			void Flush();

			TQueueList<TShared<RequestBase> > requestQueue;
			std::unordered_map<uint32_t, TShared<RequestBase> > completionMap;
			MemoryStream inputStream;
			MemoryStream outputStream;
			ITunnel::Packet currentState;
			RemoteCall& remoteCall;
			ITunnel::Connection* connection;
			size_t requestID;
		};

		friend class Session;

	public:	
		enum STATUS { CONNECTED = 0, CLOSED, ABORTED, TIMEOUT };
		RemoteCall(IThread& threadApi, ITunnel& tunnel, IFilterBase& filter, const String& entry, const TWrapper<void, bool, STATUS, const String&>& statusHandler);
		~RemoteCall();

		bool Start();
		void Reset();
		void Stop();
		void Connect(const String& target);

		ITunnel& GetTunnel() { return tunnel; }
		IFilterBase& GetFilter() { return filter; }

		template <class Input, class Output>
		void Register(const String& name, const TWrapper<bool, Output&, rvalue<Input> >& wrapper) {
			requestHandlers[name] = Response<Input, Output>(wrapper);
		}

		template <class Input, class Output>
		void Call(const TWrapper<void, rvalue<Output> >& callback, const String& name, rvalue<Input> input) {
			Session* session = outputSession();
			assert(session != nullptr);
			session->requestQueue.Push(TShared<RequestBase>::From(new Request<Input, Output>(name,
#if defined(_MSC_VER) && _MSC_VER <= 1200
				input
#else
				std::move(input)
#endif
			)));
		}

		void Flush();

	protected:
		const ITunnel::Handler OnConnection(ITunnel::Connection* connection);
		bool ThreadProc(IThread::Thread* thread, size_t context);
		void HandleEvent(ITunnel::EVENT event);
		const std::unordered_map<String, TShared<ResponseBase> >& GetRequestHandlers() const { return requestHandlers; }
		
	protected:
		IThread& threadApi;
		ITunnel& tunnel;
		IFilterBase& filter;

		String entry;
		TWrapper<void, bool, STATUS, const String&> statusHandler;
		ITunnel::Dispatcher* dispatcher;
		std::atomic<IThread::Thread*> dispThread;
		std::unordered_map<String, TShared<ResponseBase> > requestHandlers;
		std::vector<TShared<Session> > inputSessions;
		TShared<Session> outputSession;
		size_t currentResponseID;
	};
}
