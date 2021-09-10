// RemoteProxy.h
// PaintDream (paintdream@paintdream.com)
// 2016-7-15
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITunnel.h"
#include "../../Core/System/MemoryStream.h"

namespace PaintsNow {
	class TableImpl;

	enum VALUE_TYPE { TYPE_INT8 = 0, TYPE_BOOL, TYPE_INT16, TYPE_INT32, TYPE_INT64, TYPE_FLOAT, TYPE_DOUBLE, TYPE_STRING, TYPE_OBJECT, TYPE_TABLE, TYPE_ERROR };
	template <class T>
	struct TypeTraits {
		enum { Type = TYPE_ERROR };
	};

	template <>
	struct TypeTraits<bool> {
		enum { Type = TYPE_BOOL };
	};

	template <>
	struct TypeTraits<int8_t> {
		enum { Type = TYPE_INT8 };
	};

	template <>
	struct TypeTraits<uint8_t> {
		enum { Type = TYPE_INT8 };
	};

	template <>
	struct TypeTraits<int16_t> {
		enum { Type = TYPE_INT16 };
	};

	template <>
	struct TypeTraits<uint16_t> {
		enum { Type = TYPE_INT16 };
	};

	template <>
	struct TypeTraits<int32_t> {
		enum { Type = TYPE_INT32 };
	};

	template <>
	struct TypeTraits<uint32_t> {
		enum { Type = TYPE_INT32 };
	};

	template <>
	struct TypeTraits<int64_t> {
		enum { Type = TYPE_INT64 };
	};

	template <>
	struct TypeTraits<uint64_t> {
		enum { Type = TYPE_INT64 };
	};

	template <>
	struct TypeTraits<float> {
		enum { Type = TYPE_FLOAT };
	};

	template <>
	struct TypeTraits<double> {
		enum { Type = TYPE_DOUBLE };
	};

	template <>
	struct TypeTraits<String> {
		enum { Type = TYPE_STRING };
	};

	template <>
	struct TypeTraits<IScript::BaseDelegate> {
		enum { Type = TYPE_OBJECT };
	};

	template <>
	struct TypeTraits<TableImpl> {
		enum { Type = TYPE_TABLE };
	};

	class ValueBase {
	public:
		ValueBase();
		virtual ~ValueBase();
		virtual Unique QueryValueUnique() const = 0;
		virtual ValueBase* Clone() const = 0;
		virtual char GetType() const = 0;
		virtual void Reflect(IReflect& reflect) = 0;
	};

	// save raw value
	template <class T>
	class Value : public ValueBase {
	public:
		enum { Type = TypeTraits<T>::Type };
		char GetType() const override {
			assert((int)Type != TYPE_ERROR);
			return Type;
		}
		Value(const T& v) : value(v) {}
		Unique QueryValueUnique() const override {
			singleton Unique type = UniqueType<T>::Get();
			return type;
		}

		void Reflect(IReflect& reflect) override {
			ReflectProperty(value);
		}

		ValueBase* Clone() const override {
			return new Value(value);
		}

		T value;
	};

	template <>
	class Value<const char*> : public ValueBase {
	public:
		enum { Type = TypeTraits<String>::Type };
		char GetType() const override {
			return Type;
		}
		Value(const char*& t) : value(t) {}
		Unique QueryValueUnique() const override {
			singleton Unique u = UniqueType<String>::Get();;
			return u;
		}

		ValueBase* Clone() const override {
			return new Value<String>(value);
		}

		void Reflect(IReflect& reflect) override {
			if (reflect.IsReflectProperty()) {
				ReflectProperty(value);
			}
		}

		String value;
	};

	class Variant : public TReflected<Variant, IReflectObjectComplex> {
	public:
		Variant() : value(nullptr) {}

		template <class T>
		static ValueBase* Construct(const T& object) {
			char* buffer = new char[sizeof(Value<T>) + sizeof(size_t)];
			ValueBase* value = new(buffer + sizeof(size_t)) Value<T>(object);
			*(size_t*)buffer = 1;
			return value;
		}

		template <class T>
		Variant(const T& object) {
			value = Construct(object);
		}

		inline void AddRef() {
			if (value != nullptr) {
				size_t* ref = (size_t*)((char*)value - sizeof(size_t));
				(*ref)++;
			}
		}

		void Destroy() override {
			if (value != nullptr) {
				size_t* ref = (size_t*)((char*)value - sizeof(size_t));
				assert(*ref < 0x100);
				if (--(*ref) == 0) {
					value->~ValueBase(); // call destructor manually
					delete[](char*)ref;
				}
			}
		}

		Variant(const Variant& var);
		Variant& operator = (const Variant& var);

		~Variant() override;

		ValueBase* Get() const {
			return value;
		}

		ValueBase* operator -> () const {
			return value;
		}

		TObject<IReflect>& operator () (IReflect& reflect) override;
	private:
		ValueBase* value;
	};

	class TableImpl {
	public:
		std::vector<Variant> arrayPart;
		std::map<String, Variant> mapPart;
	};
	
	template <>
	class Value<TableImpl> : public ValueBase {
	public:
		enum { Type = TypeTraits<TableImpl>::Type };

		char GetType() const override {
			return Type;
		}

		Value() {}
		Value(const TableImpl& t) : value(t) {}

		Unique QueryValueUnique() const override {
			singleton Unique u = UniqueType<TableImpl>::Get();
			return u;
		}

		ValueBase* Clone() const override {
			return new Value<TableImpl>(value);
		}

		void Reflect(IReflect& reflect) override;

		TableImpl value;
	};

	class RemoteFactory : public TReflected<RemoteFactory, IReflectObjectComplex> {
	public:
		void Initialize(IScript::Request& request, const TWrapper<void, IScript::Request&, IReflectObject&, const IScript::Request::Ref&>& callback);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, const String&> NewObject;
		TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, IScript::BaseDelegate> QueryObject;
		IScript::Request::Ref globalRef;
	};

	class RemoteProxy : public TReflected<RemoteProxy, IReflectObjectComplex>, public IScript {
	public:
		enum STATUS { CONNECTED = 0, CLOSED, ABORTED, TIMEOUT };
		RemoteProxy(IThread& threadApi, ITunnel& tunnel, const TWrapper<IScript::Object*, const String&>& creator, const String& entry, const TWrapper<void, IScript::Request&, bool, STATUS, const String&>& statusHandler = nullptr);
		~RemoteProxy() override;
		virtual void SetEntry(const String& entry);
		virtual void Reconnect(IScript::Request& request);
		bool IsClosing() const override;
		bool IsHosting() const override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		struct ObjectInfo {
			ObjectInfo();
			~ObjectInfo();

			struct Entry {
				std::pair<IScript::Request::Ref, size_t> CallFilter(IScript::Request& request, bool pre) {
					return std::make_pair(IScript::Request::Ref(reinterpret_cast<size_t>((void*)&obj)), index);
				}

				Entry();
				~Entry();
				Entry(const Entry& rhs);
				Entry& operator = (const Entry& rhs);

				String name;
				IReflect::Param retValue;
				std::vector<IReflect::Param> params;
				IScript::Request::AutoWrapperBase* wrapper;
				TWrapper<std::pair<IScript::Request::Ref, size_t>, IScript::Request&, bool> method;
				IScript::BaseDelegate obj;
				size_t index;
			};

			int64_t refCount;
			std::vector<Entry> collection;
			bool needQuery;
		};

		class Request : public TReflected<Request, IScript::Request> {
		public:
			Request(RemoteProxy& host, ITunnel::Connection* connection, const TWrapper<void, IScript::Request&, bool, STATUS, const String&>& statusHandler);
			~Request() override;
			void Attach(ITunnel::Connection* connnection);
			void Reconnect();

		public:
			int GetCount() override;
			IScript* GetScript() override;

			void QueryInterface(const TWrapper<void, IScript::Request&, IReflectObject&, const Ref&>& callback, IReflectObject& target, const Ref& g) override;

			bool Call(const AutoWrapperBase& defer, const Request::Ref& g) override;
			TYPE GetCurrentType() override;
			IScript::Request::Ref Load(const String& script, const String& pathname = String()) override;
			IScript::Request& Push() override;
			IScript::Request& Pop() override;
			virtual IScript::Request& CleanupIndex();
			IScript::Request& operator >> (IScript::Request::Arguments&) override;
			IScript::Request& operator >> (Ref&) override;
			IScript::Request& operator << (const Ref&) override;
			IScript::Request& operator << (const Nil&) override;
			IScript::Request& operator << (const BaseDelegate&) override;
			IScript::Request& operator >> (BaseDelegate&) override;
			IScript::Request& operator << (const Global&) override;

			IScript::Request& operator << (const TableStart&) override;
			IScript::Request& operator >> (TableStart&) override;
			IScript::Request& operator << (const TableEnd&) override;
			IScript::Request& operator << (const ArrayStart&) override;
			IScript::Request& operator >> (ArrayStart&) override;
			IScript::Request& operator << (const ArrayEnd&) override;
			IScript::Request& operator << (const Key&) override;
			IScript::Request& operator >> (Iterator&) override;
			IScript::Request& operator << (double value) override;
			IScript::Request& operator >> (double& value) override;
			IScript::Request& operator << (const StringView& str) override;
			IScript::Request& operator >> (StringView& str) override;
			IScript::Request& operator << (const String& str) override;
			IScript::Request& operator >> (String& str) override;
			IScript::Request& operator << (const Bytes& str) override;
			IScript::Request& operator >> (Bytes& str) override;
			IScript::Request& operator << (const char* str) override;
			IScript::Request& operator >> (const char*& str) override;
			IScript::Request& operator << (bool b) override;
			IScript::Request& operator >> (bool& b) override;

			IScript::Request& operator << (const AutoWrapperBase& wrapper) override;
			IScript::Request& operator << (int64_t u) override;
			IScript::Request& operator >> (int64_t& u) override;

			virtual bool IsValid(const BaseDelegate& d);
			Ref Reference(const Ref& d) override;
			TYPE GetReferenceType(const Ref& d) override;
			void Dereference(Ref& ref) override;
			AutoWrapperBase* GetWrapper(const Ref& ref) override;
			IScript::Request& MoveVariables(IScript::Request& target, size_t count) override;

			class Packet : public TReflected<Packet, IReflectObjectComplex> {
			public:
				Packet();
				TObject<IReflect>& operator () (IReflect& reflect) override;
				int64_t object; // 0 for global routines
				int64_t procedure;
				int64_t callback;
				bool response;
				bool deferred;
				std::vector<Variant> vars;
				std::vector<std::pair<int64_t, int64_t> > localDelta;
				std::vector<std::pair<int64_t, int64_t> > remoteDelta;
			};

			void OnConnection(ITunnel::EVENT event);
			void Process();
			void ProcessPacket(Packet& packet);
			void PostPacket(Packet& packet);
			void Run();
			IScript::Request::Ref ReferenceEx(const IScript::BaseDelegate* base);
			void DereferenceEx(IScript::BaseDelegate* base);
			void ApplyDelta(std::map<IScript::Object*, ObjectInfo>& info, const std::vector<std::pair<int64_t, int64_t> >& delta, bool retrieve);
			void QueryObjectInterface(ObjectInfo& objectInfo, const IScript::BaseDelegate& d, const TWrapper<void, IScript::Request&, IReflectObject&, const Ref&>& callback, IReflectObject& target, const Request::Ref& g);

		public:
			void RequestNewObject(IScript::Request& request, const String& url);
			void RequestQueryObject(IScript::Request& request, IScript::BaseDelegate base);

			TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			RemoteProxy& host;
			IThread& threadApi;

			std::vector<Variant> buffer;
			String key;
			int idx;
			int initCount;
			int tableLevel;
			ITunnel::Connection* connection;

			ObjectInfo globalRoutines;
			std::set<IScript::Request::AutoWrapperBase*> localCallbacks;
			std::set<IReflectObject*> tempObjects;
			std::map<IScript::Object*, size_t> localObjectRefDelta;
			std::map<IScript::Object*, size_t> remoteObjectRefDelta;
			std::map<IScript::Object*, ObjectInfo> localActiveObjects;
			std::map<IScript::Object*, ObjectInfo> remoteActiveObjects;
			std::set<IScript::BaseDelegate*> localReferences;
			TWrapper<void, IScript::Request&, bool, STATUS, const String&> statusHandler;
			IThread::Event* syncCallEvent;
			ITunnel::Packet state;
			enum { CHUNK_SIZE = 0x1000 };
			MemoryStream stream;

			bool manually;
			bool isKey;
		};

		friend class Request;

		const char* GetFileExt() const override;
		IScript* NewScript() const override;
		IScript::Request* NewRequest(const String& entry) override;
		IScript::Request& GetDefaultRequest() override;

		const TWrapper<IScript::Object*, const String&>& GetObjectCreator() const;
		bool Run();
		void Reset() override;
		void Stop();

	protected:
		const ITunnel::Handler OnConnection(ITunnel::Connection* connection);
		bool ThreadProc(IThread::Thread* thread, size_t context);
		void HandleEvent(ITunnel::EVENT event);

	protected:
		ITunnel& tunnel;
		TWrapper<IScript::Object*, const String&> objectCreator;
		TWrapper<void, IScript::Request&, bool, STATUS, const String&> statusHandler;
		Request defaultRequest;
		ITunnel::Dispatcher* dispatcher;
		std::atomic<IThread::Thread*> dispThread;
		ObjectInfo globalRoutines;
		String entry;
	};
}
