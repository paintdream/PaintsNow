// RemoteCall.h
// PaintDream (paintdream@paintdream.com)
// 2016-7-15
//

#pragma once
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../General/Interface/ITunnel.h"
#include "../../Core/System/Tiny.h"
#include "../../Core/Template/TQueue.h"

namespace PaintsNow {
	class RemoteCall {
	protected:
		class Handler : public SharedTiny {
		public:
			virtual void Handle(IFilterBase& filter, size_t context, IStreamBase& inputStream) = 0;
			virtual const String& GetName() const {
				static String empty;
				return empty;
			}
		};

		template <class Input>
		class Request : public TReflected<Request<Input>, Handler> {
			Request(const String& n, rvalue<Input> input) : name(n), 
#if defined(_MSC_VER) && _MSC_VER <= 1200
				inputPacket(input)
#else
				inputPacket(std::move(input))
#endif
			{}

			const String& GetName() const override {
				return name;
			}

			void Handle(RemoteCall& remoteCall, size_t context, IStreamBase& inputStream) override {
				IStreamBase* inputEncodeStream = remoteCall.GetFilter().CreateFilter(inputStream);
				*inputEncodeStream << inputPacket;
				inputEncodeStream->Destroy();
			}

			String name;
			Input inputPacket;
		};

		template <class Input, class Output>
		class Response : public TReflected<Response<Input, Output>, Handler> {
		public:
			Response(const TWrapper<bool, Output&, rvalue<Input> >& handler) : wrapper(handler) {}

			void Handle(RemoteCall& remoteCall, size_t context, IStreamBase& inputStream) override {
				Input inputPacket;
				Output outputPacket;

				IStreamBase* inputEncodeStream = remoteCall.GetFilter().CreateFilter(inputStream);
				*inputEncodeStream >> inputPacket;
				inputEncodeStream->Destroy();

				if (wrapper(outputPacket, std::move(outputPacket))) {
					// sync
					remoteCall.Complete(context, outputPacket);
				} else {
					// async, should call remoteCall.Complete() by yourself
				}
			}

			TWrapper<bool, Output&, rvalue<Input> > wrapper;
		};

	public:	
		enum STATUS { CONNECTED = 0, CLOSED, ABORTED, TIMEOUT };
		RemoteCall(IThread& threadApi, ITunnel& tunnel, IFilterBase& filter, const String& entry, const TWrapper<void, bool, STATUS, const String&>& statusHandler);
		~RemoteCall();

		bool Run();
		void Reset();
		void Stop();
		void Connect(const String& target);

		template <class Input, class Output>
		void Register(const String& name, const TWrapper<bool, Output&, rvalue<Input> >& wrapper) {
			requestHandlers[name] = Response<Input, Output>(wrapper);
		}

		template <class Input, class Output>
		void Call(const TWrapper<void, rvalue<Output> >& callback, const String& name, rvalue<Input> input) {
			requestQueue.Push(TShared<Handler>::From(new Request<Input>(
#if defined(_MSC_VER) && _MSC_VER <= 1200
				input
#else
				std::move(input)
#endif
			)));

			Flush();
		}

		template <class Output>
		void Complete(Output& outputPacket) {
		}

		forceinline IFilterBase& GetFilter() {
			return filter;
		}

		void Flush();

	protected:
		const ITunnel::Handler OnConnection(ITunnel::Connection* connection);
		bool ThreadProc(IThread::Thread* thread, size_t context);
		void HandleEvent(ITunnel::EVENT event);

	protected:
		IThread& threadApi;
		ITunnel& tunnel;
		IFilterBase& filter;

		String entry;
		TWrapper<void, bool, STATUS, const String&> statusHandler;
		ITunnel::Dispatcher* dispatcher;
		std::atomic<IThread::Thread*> dispThread;
		std::unordered_map<String, TShared<Handler> > requestHandlers;
		TQueueList<TShared<Handler> > requestQueue;
		std::vector<ITunnel::Connection*> inConnections;
		ITunnel::Connection* outConnection;
		size_t currentIndex;
	};
}
