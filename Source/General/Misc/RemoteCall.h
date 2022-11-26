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
	class ProtoInput : public TReflected<ProtoInput, IReflectObjectComplex> {};
	class ProtoOutput : public TReflected<ProtoOutput, IReflectObjectComplex> {};

	class RemoteCall {
	protected:
		class RequestBase : public SharedTiny {
		public:
			virtual void Prepare(IStreamBase& stream) = 0;
			virtual void Complete(RemoteCall&, IStreamBase& stream) = 0;

			String name;
		};

	public:
		class Session : public TReflected<Session, SharedTiny> {
		public:
			friend class RemoteCall;
			
			enum
			{
				SESSION_PASSIVE_CONNECTION = TINY_CUSTOM_BEGIN,
				SESSION_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN << 1	
			};

			Session(RemoteCall& remoteCall);
			~Session() override;

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

				tunnel.WriteConnection(connection, serializeStream.GetBuffer(), serializeStream.GetOffset(), 0);
			}

			template <class Input, class Output>
			void Invoke(const String& name, rvalue<Input> input, const TWrapper<void, RemoteCall&, rvalue<Output> >& callback) {
				requestQueue.Push(TShared<RequestBase>::From(new Request<Input, Output>(name, std::move(input), callback)));
			}

			ITunnel::Connection* GetConnection() const;
			bool IsPassive() const;

		protected:
			virtual void HandleEvent(ITunnel::Connection*, ITunnel::EVENT event);

		protected:
			RemoteCall& remoteCall;
			TQueueList<TShared<RequestBase> > requestQueue;
			std::unordered_map<uint32_t, TShared<RequestBase> > completionMap;
			MemoryStream inputStream;
			MemoryStream outputStream;
			ITunnel::Connection* connection;
			uint32_t requestID;
		};

		template <class Input, class Output>
		class Request : public TReflected<Request<Input, Output>, RequestBase> {
		public:
			Request(const String& n, rvalue<Input> input, const TWrapper<void, RemoteCall&, rvalue<Output> >& callback) : wrapper(callback), inputPacket(std::move(input))
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
			virtual bool Handle(RemoteCall& remoteCall, IStreamBase& outputStream, IStreamBase& inputStream, const TShared<Session>& context, uint32_t id) = 0;
		};

		template <class Input, class Output>
		class Response : public TReflected<Response<Input, Output>, ResponseBase> {
		public:
			Response(const TWrapper<bool, RemoteCall&, Output&, rvalue<Input>, const TShared<Session>&, uint32_t>& handler) : wrapper(handler) {}

			bool Handle(RemoteCall& remoteCall, IStreamBase& outputStream, IStreamBase& inputStream, const TShared<Session>& context, uint32_t id) override {
				inputStream >> inputPacket;
				bool sync = wrapper(remoteCall, outputPacket, std::move(inputPacket), context, id);
				if (sync) {
					outputStream << std::move(outputPacket);
				}

				return sync;
			}

			TWrapper<bool, RemoteCall&, Output&, rvalue<Input>, const TShared<Session>&, uint32_t> wrapper;
			Input inputPacket;
			Output outputPacket;
		};

		friend class MetaRemoteMethod;

	public:	
		RemoteCall(IThread& threadApi, ITunnel& tunnel, IFilterBase& filter, const String& entry = "");
		~RemoteCall();

		bool Start();
		void Reset();
		void Stop();

		TShared<Session> Connect(const String& target);
		ITunnel& GetTunnel() { return tunnel; }
		IFilterBase& GetFilter() { return filter; }
		const std::unordered_map<String, TShared<ResponseBase> >& GetRequestHandlers() const { return requestHandlers; }
		IThread& GetThreadApi() { return threadApi; }

		// Compatible for VC6
#if defined(_MSC_VER) && _MSC_VER <= 1200
		template <class F>
		void Register(const String& name, const F& wrapper) {
			RegisterByResponse(name, TShared<ResponseBase>::From(new Response<std::remove_reference<F::_C>::type, std::remove_reference<F::_B>::type>(wrapper)));
		}
#else
		template <class Input, class Output>
		void Register(const String& name, const TWrapper<bool, RemoteCall&, Output&, rvalue<Input>, const TShared<Session>&, uint32_t>& wrapper) {
			RegisterByResponse(name, TShared<ResponseBase>::From(new Response<Input, Output>(wrapper)));
		}
#endif
		void RegisterByObject(const String& prefix, IReflectObject& object);
		void RegisterByResponse(const String& name, const TShared<ResponseBase>& response);

	protected:
		bool ThreadProc(IThread::Thread* thread, size_t context);
		
	protected:
		// extentable processor
		virtual TShared<Session> CreateSession();
		virtual void HandleEvent(ITunnel::Listener* listener, ITunnel::EVENT event);
		virtual const TWrapper<void, ITunnel::Connection*, ITunnel::EVENT> OnConnection(ITunnel::Connection* connection);
		virtual void AddSession(const TShared<Session>& session);
		virtual void RemoveSession(const TShared<Session>& session);
		friend class Session;
		
	protected:
		IThread& threadApi;
		ITunnel& tunnel;
		IFilterBase& filter;

		String entry;
		ITunnel::Dispatcher* dispatcher;
		std::atomic<IThread::Thread*> dispThread;
		std::unordered_map<String, TShared<ResponseBase> > requestHandlers;
		std::vector<TShared<Session> > sessionHolders;
		size_t currentResponseID;
	};

	class MetaRemoteMethod : public TReflected<MetaRemoteMethod, MetaNodeBase> {
	public:
		MetaRemoteMethod(const String& k = "") : key(k) {}
		MetaRemoteMethod operator = (const String& k) {
			return MetaRemoteMethod(k);
		}

		class TypedBase : public TReflected<TypedBase, MetaNodeBase> {
		public:
			String key;
			TShared<RemoteCall::ResponseBase> response;
		};

		// For vc6 compat
		class Type : public TReflected<Type, TypedBase> {};
		template <class T, class D>
		class Typed : public TypedBase {};

#if defined(_MSC_VER) && _MSC_VER <= 1200
		template <class T, class D>
		Type FilterField(T* pointer, D* member) const {
			return FilterFieldImpl(Wrap(pointer, *member));
		}

		template <class F>
		Type FilterFieldImpl(const F& wrapper) const {
			Type type;
			type.key = key;
			type.response = TShared<RemoteCall::ResponseBase>::From(new RemoteCall::Response<std::decay<F::_C>::type, std::decay<F::_B>::type>(wrapper));
			return type;
		}

		template <class T, class D>
		struct RealType {
			typedef Typed<T, D> Type;
		};
#else
		template <class T, class D>
		TypedBase FilterField(T* pointer, D* member) const {
			return FilterFieldImpl(Wrap(pointer, *member));
		}

		template <class Output, class Input>
		TypedBase FilterFieldImpl(const TWrapper<bool, RemoteCall&, Output&, rvalue<Input>, const TShared<RemoteCall::Session>&, uint32_t>& wrapper) const {
			TypedBase type;
			type.key = key;
			type.response = TShared<RemoteCall::ResponseBase>::From(new RemoteCall::Response<Input, Output>(wrapper));
			return type;
		}

		template <class T, class D>
		struct RealType {
			typedef TypedBase Type;
		};
#endif

		String key;
	};

	extern MetaRemoteMethod RemoteMethod;
}
