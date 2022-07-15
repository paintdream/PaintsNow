// IReflect.h
// PaintDream (paintdream@paintdream.com)
// 2014-7-16
//

#pragma once
#include "../PaintsNow.h"
#include "IType.h"
#include "IThread.h"
#include "../Template/TObject.h"
#include "../Template/TProxy.h"
#include "../Template/TAtomic.h"
#include "../Template/TMap.h"
#include <vector>
#include <cstring>
#include <map>
#include <cassert>
#include <iterator>
#ifndef _MSC_VER
#include <typeinfo>
#endif

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4172)
#endif

namespace PaintsNow {
	class IStreamBase;
	class IReflectObject;

	// Runtime class info
	class UniqueAllocator;
	class_aligned(64) UniqueInfo {
	public:
		UniqueInfo() : size(0), alignment(0), allocator(nullptr) { critical.store(0, std::memory_order_relaxed); }
		~UniqueInfo();
		size_t GetSize() const { return size; }
		size_t GetAlignment() const { return alignment; }
		const String& GetName() const { return typeName; } // Get full name (demangled)
		IReflectObject* Create() const { return creator(); } // Create instance, notice that not all uniques support this action
		IReflectObject* Clone(const IReflectObject& src) const { return cloner(src); } // Create instance, notice that not all uniques support this action
		bool IsCreatable() const { return creator; } // Check if it supports creation
		bool IsCloneable() const { return cloner; } // Check if it supports clone
		String GetBriefName() const; // Get brief name, without namespace specifier
		UniqueAllocator* GetAllocator() const { return allocator; } // Get the allocator of this type, note that not all uniques share the same allocator.

		// Check if it is derived from unique specified by `info`
		bool IsClass(UniqueInfo* info) const {
			return info == this || BinaryFind(interfaces.begin(), interfaces.end(), info) != interfaces.end();
		}

		friend class UniqueAllocator;

	protected:
		size_t size;
		size_t alignment;
		UniqueAllocator* allocator;
		String typeName;

	public:
		TWrapper<IReflectObject*> creator;
		TWrapper<IReflectObject*, const IReflectObject&> cloner;
		std::vector<KeyValue<UniqueInfo*, size_t> > interfaces; // Derivations, managed with flatmap
		std::atomic<size_t> critical;
	};

	// Allocator that allocates uniques.
	class UniqueAllocator {
	public:
		UniqueAllocator();
		virtual ~UniqueAllocator();

		// Return the global allocator.
		static UniqueAllocator& GetInstance();
		static void SetInstance(UniqueAllocator* uniqueAllocator);
		// Create unique structure
		virtual UniqueInfo* Create(const String& name, size_t size = 0, size_t alignment = 0);
		friend class UniqueInfo;

	protected:
		UniqueAllocator(const UniqueAllocator& rhs);
		std::atomic<int32_t> critical;
		std::unordered_map<String, UniqueInfo*> mapType;
	};

	// Quick wrapper for runtime class info
	class Unique {
	public:
		Unique(UniqueInfo* f = nullptr) : info(f) {}
		Unique(const String& name) : info(UniqueAllocator::GetInstance().Create(name)) {}

		bool operator == (const Unique& unique) const { return info == unique.info; }
		bool operator != (const Unique& unique) const { return info != unique.info; }
		bool operator < (const Unique& unique) const { return info < unique.info; }
		const UniqueInfo* operator -> () const { return info; }
		UniqueInfo* operator -> () { return info; }
		operator UniqueInfo* () const { return info; }
		operator bool() const { return info != nullptr; }
		UniqueInfo* GetInfo() const { return info; }

	private:
		UniqueInfo* info;
	};

	// Generic UniqueType<> for template induction
	template <size_t n>
	struct AlignHelper {
		enum { value = n };
	};

	template <class T>
	struct AlignOf : public AlignHelper<alignof(T)> {};

	template <>
	struct AlignOf<void> : public AlignHelper<1> {};

	template <class T>
	struct UniqueType : public TypeTrait<T> {
		static String Demangle(const char* name) {
			String className;
#ifdef __GNUG__
			int status = -4; // some arbitrary value to eliminate the compiler warning
			// enable c++11 by passing the flag -std=c++11 to g++
			std::unique_ptr<char, void(*)(void*)> res{
				abi::__cxa_demangle(name, nullptr, nullptr, &status),
					std::free
			};

			className = (status == 0) ? res.get() : name;
#else
			size_t len = strlen(name);
			className = String(name, len > 1024 ? 1024 : len);
#endif

			// remove 'class ' or 'struct ' prefix
			const String skipPrefixes[] = { "class ", "struct " };
			for (size_t i = 0; i < sizeof(skipPrefixes) / sizeof(skipPrefixes[0]); i++) {
				RemovePatterns(className, skipPrefixes[i]);
			}

			return className == "PaintsNow::Void" ? "void" : className;
		}

		static void RemovePatterns(String& s, const String& p) {
			size_t size = p.size();
			for (size_t i = s.find(p); i != String::npos; i = s.find(p))
				s.erase(i, size);
		}

		class InfoHolder {
		public:
			InfoHolder() {
				UniqueAllocator& allocator = UniqueAllocator::GetInstance();
				info = allocator.Create(Demangle(typeid(T).name()), sizeof(typename ReturnType<T>::type), AlignOf<T>::value);
			}

			UniqueInfo* info;
		};

		static Unique Get() {
			return TSingleton<InfoHolder>::Get().info;
		}
	};

	class IReflect;
	class IIterator;

	// Reflectee
	class IReflectObject : public TObject<IReflect> {
	public:
		IReflectObject();
		// General object construction
		// We can initialize an IReflectObject with anything, and get a stub IReflectObject
		//	 that IsBasicObject() == true && IsIterator() == false
		// It's useful when writing templates dealing with various pod types, see IStreamBase for more details
		template <class T>
		IReflectObject(const T& t) {}

		~IReflectObject() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual const TObject<IReflect>& operator () (IReflect& reflect) const; // Just forward to non-const one 
		virtual bool IsBasicObject() const; // returns true, any sub-classes should return false;
		virtual bool IsIterator() const; // only for IIterators
		virtual Unique GetUnique() const; // get unique of this object
		virtual IReflectObject* Clone() const; // clone this object, the default behaviour is not implemented
		virtual String ToString() const; // convert this to string, usually a string that represents the type

		// Query interface, may be slow.
		// Same purpose as dynamic_cast<>
		template <class T>
		static T* QueryInterfaceEx(IReflectObject* object, Unique unique, UniqueType<T> target) {
			if (unique == target.Get()) return reinterpret_cast<T*>(object);

			const std::vector<KeyValue<UniqueInfo*, size_t> >& vec = unique->interfaces;
			UniqueInfo* targetInfo = target.Get();
			std::vector<KeyValue<UniqueInfo*, size_t> >::const_iterator it = BinaryFind(vec.begin(), vec.end(), targetInfo);

			// We do not support virtual inheritance
			return it != vec.end() ? reinterpret_cast<T*>((uint8_t*)object + it->second) : nullptr;
		}

		// Query specified interface. Usage: T* inter = p->QueryInterface(UniqueType<T>());
		// This is a workaround for old compilers that do not support p->QueryInterface<T>();
		template <class T>
		T* QueryInterface(UniqueType<T> t) {
			return QueryInterfaceEx(this, GetUnique(), t);
		}

		template <class T>
		const T* QueryInterface(UniqueType<T> t) const {
			return QueryInterfaceEx(const_cast<IReflectObject*>(this), GetUnique(), t);
		}

		virtual void Destroy();

#if defined(_MSC_VER) && _MSC_VER <= 1200
		// These two functions are work-arounds for msvc compiler.
		static const IReflectObject& TransformReflectObject(const IReflectObject& t) {
			return t.IsBasicObject() ? TSingleton<IReflectObject>::Get() : t;
		}

		static const IReflectObject& TransformReflectObject(IReflectObject& t) {
			return t;
		}
#endif
		
		// Slow path, not recommended for frequently calls
		template <class T>
		T* Inspect(UniqueType<T> unique, const String& key = "") {
			singleton Unique u = UniqueType<T>::Get();
			return reinterpret_cast<T*>(InspectEx(u, key));
		}

		virtual void* InspectEx(Unique unique, const String& key);

		// Serialization
		virtual bool operator >> (IStreamBase& stream) const;
		virtual bool operator << (IStreamBase& stream);
	};

	// Non-pods base object
	class IReflectObjectComplex : public IReflectObject {
	public:
		bool IsBasicObject() const override;
		virtual size_t ReportMemoryUsage() const;

		String ToString() const override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
	};

	// IIterator is for all linear container iterators
	class IIterator : public IReflectObject {
	public:
		IIterator();
		~IIterator() override;
		String ToString() const override;
		bool IsBasicObject() const override;
		bool IsIterator() const override;
		virtual IIterator* New() const = 0; // create new iterator
		virtual void Attach(void* base) = 0; // attach to an existing instance
		virtual void* GetHost() const = 0; // get instanced attached
		virtual void Initialize(size_t count) = 0; // initialize container with specified count of elements
		virtual size_t GetTotalCount() const = 0; // get count of elements
		virtual void* Get() = 0; // get current element address
		virtual Unique GetElementUnique() const = 0; // get unique of element
		virtual Unique GetElementReferenceUnique() const = 0; // get reference unique of element, usually used on container that hold pointers
		virtual const IReflectObject& GetElementPrototype() const = 0;
		virtual bool IsElementBasicObject() const = 0;
		virtual bool IsLayoutLinear() const = 0; // check if the memory layout is linear
		virtual bool IsLayoutPinned() const = 0; // check if it is addressable from prototype element
		virtual bool Next() = 0; // iterate next element
	};

	// IIterator for vector arrays
	template <class T>
	class VectorIterator : public IIterator {
	public:
#if defined(_MSC_VER) && _MSC_VER <= 1200
		typedef T::value_type value_type;
#else
		typedef typename T::value_type value_type;
#endif
		VectorIterator(T* p) : base(p), i(0) {}
		void Initialize(size_t c) override {
			assert(base != nullptr);
			base->resize(c);
		}

		size_t GetTotalCount() const override {
			assert(base != nullptr);
			return base->size();
		}

		Unique GetElementUnique() const override {
			return UniqueType<value_type>::Get();
		}

		Unique GetElementReferenceUnique() const override {
#if defined(_MSC_VER) && _MSC_VER <= 1200
			return UniqueType<std::remove_pointer<value_type>::type>::Get();
#else
			return UniqueType<typename std::remove_pointer<value_type>::type>::Get();
#endif
		}

		bool IsElementBasicObject() const override {
#if defined(_MSC_VER) && _MSC_VER <= 1200
			const value_type t = value_type();
			const IReflectObject& obj = TransformReflectObject(t);

			return (void*)&obj != (void*)&t;
#else
			return !std::is_base_of<IReflectObject, value_type>::value;
#endif
		}

		const IReflectObject& GetElementPrototype() const override {
			assert(!IsElementBasicObject());
			static const value_type t = value_type();
#if defined(_MSC_VER) && _MSC_VER <= 1200
			return TransformReflectObject(t);
#else
			static typename std::conditional<std::is_base_of<IReflectObject, value_type>::value, const IReflectObject&, const IReflectObject>::type v = t;
			return v;
#endif
		}

		void* Get() override {
			assert(i != 0);
			return &(*base)[i - 1];
		}

		bool Next() override {
			if (i >= (*base).size()) {
				return false;
			}

			i++;
			return true;
		}

		IIterator* New() const override {
			return new VectorIterator(base);
		}

		void Attach(void* p) override {
			base = reinterpret_cast<T*>(p);
			i = 0;
		}

		bool IsLayoutLinear() const override {
			return true;
		}

		bool IsLayoutPinned() const override {
			return false;
		}

		void* GetHost() const override {
			return base;
		}

		Unique GetUnique() const override {
			typedef VectorIterator<T> Class;
			return UniqueType<Class>::Get();
		}

	private:
		T* base;
		size_t i;
	};

	// Powerful meta annotation support
	// We can attach more optional customized data (metadata) when reflecting a property/method/class.
	// Such as serialization state, script binding state, event doc string.
	// These metadata are protocols between reflector and reflectees.
	//

	class MetaNodeBase : public IReflectObjectComplex {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			return *this;
		}
	};

	// MetaChain is often constructed by cascaded [][] operators
	// So they are usually temporary allocated objects
	// Be aware of thier lifetime!
	class MetaChainBase : public IReflectObjectComplex {
	public:
		virtual const MetaChainBase* GetNext() const {
			return nullptr;
		}

		virtual const MetaNodeBase* GetNode() const {
			return nullptr;
		}

		virtual const MetaNodeBase* GetRawNode() const {
			return nullptr;
		}

	private:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			assert(false);
			return *this;
		}
	};

	// Implement meta chain structure
	template <class T, class D, class K, class Base>
	class MetaChain : public MetaChainBase {
	public:
		MetaChain(const K& k, const Base& b) : chainNode(k), rawChainNodePtr(&k), base(b) {}

#if defined(_MSC_VER) && _MSC_VER <= 1200
		template <class N>
		inline MetaChain<T, D, N::Type, MetaChain<T, D, K, Base> > operator [] (const N& t) const {
			return MetaChain<T, D, N::Type, MetaChain<T, D, K, Base> >(t.FilterField(GetObject(), GetMember()), *this);
		}
#else
		template <class N>
		inline MetaChain<T, D, typename N::template RealType<T, D>::Type, MetaChain<T, D, K, Base> > operator [] (const N& t) const {
			return MetaChain<T, D, typename N::template RealType<T, D>::Type, MetaChain<T, D, K, Base> >(t.FilterField(GetObject(), GetMember()), *this);
		}
#endif

		inline bool IsChain() const {
			return true;
		}

		const MetaChainBase* GetNext() const override {
			return base.IsChain() ? &base : nullptr;
		}

		const MetaNodeBase* GetNode() const override {
			return &chainNode;
		}

		const MetaNodeBase* GetRawNode() const override {
			return rawChainNodePtr;
		}

		// Finalize all meta chain.
		inline void operator * () const {
			Finish(this);
		}

		void Finish(const MetaChainBase* head) const {
			(const_cast<Base&>(base)).Finish(head);
		}

		inline T* GetObject() const {
			return base.GetObject();
		}

		inline D* GetMember() const {
			return base.GetMember();
		}

		K chainNode;
		const K* rawChainNodePtr;
		const Base& base;
	};

	// Reflector
	class pure_interface IReflect {
	public:
		IReflect(bool reflectProperty = true, bool reflectMethod = false, bool reflectClass = true, bool reflectEnum = false);
		virtual ~IReflect();

		bool IsReflectProperty() const;
		bool IsReflectMethod() const;
		bool IsReflectClass() const;
		bool IsReflectEnum() const;

		void RegisterBuiltinTypes(bool useStdintType = false);
		class Param {
		public:
			Param(Unique t = UniqueType<Void>::Get(), Unique d = UniqueType<Void>::Get(), bool re = false, bool co = false) : type(t), decayType(d), isReference(re), isConst(co) {}
			operator Unique () const {
				return type;
			}

			Unique type;
			Unique decayType;
			String name;
			bool isReference;
			bool isConst;
		};

		// For enum reflection
		template <class T>
		inline void OnEnum(T& t, const char* name, const MetaChainBase* meta) {
			singleton Unique u = UniqueType<T>::Get();
			Enum(verify_cast<size_t>(t), u, name, meta);
		}

		// For class reflection
		template <class T>
		inline void OnClass(T& t, const char* name, const char* path, const MetaChainBase* meta) {
			singleton Unique u = UniqueType<T>::Get();
			Class(t, u, name, path, meta);
		}

		// For property reflection
		template <class T>
		inline void OnProperty(const T& t, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
			singleton Unique u = UniqueType<T>::Get();
#if defined(_MSC_VER) && _MSC_VER <= 1200
			singleton Unique ur = UniqueType<std::remove_pointer<T>::type>::Get();
#else
			singleton Unique ur = UniqueType<typename std::remove_pointer<T>::type>::Get();
#endif
			ForwardProperty(const_cast<T&>(t), u, ur, name, base, ptr, meta);
		}

		inline void ForwardProperty(const IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
			Property(const_cast<IReflectObject&>(s), typeID, refTypeID, name, base, ptr, meta);
		}

#if (defined(_MSC_VER) && _MSC_VER < 1800)
		template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
		inline void OnMethod(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& t, const char* name, const MetaChainBase* meta) {
			static Unique params[] = {
				UniqueType<A>::Get(),
				UniqueType<B>::Get(),
				UniqueType<C>::Get(),
				UniqueType<D>::Get(),
				UniqueType<E>::Get(),
				UniqueType<F>::Get(),
				UniqueType<G>::Get(),
				UniqueType<H>::Get(),
				UniqueType<I>::Get(),
				UniqueType<J>::Get(),
				UniqueType<K>::Get(),
				UniqueType<L>::Get(),
				UniqueType<M>::Get(),
				UniqueType<N>::Get(),
				UniqueType<O>::Get(),
				UniqueType<P>::Get()
			};

			static Unique decayParams[] = {
				UniqueType<typename std::decay<A>::type>::Get(),
				UniqueType<typename std::decay<B>::type>::Get(),
				UniqueType<typename std::decay<C>::type>::Get(),
				UniqueType<typename std::decay<D>::type>::Get(),
				UniqueType<typename std::decay<E>::type>::Get(),
				UniqueType<typename std::decay<F>::type>::Get(),
				UniqueType<typename std::decay<G>::type>::Get(),
				UniqueType<typename std::decay<H>::type>::Get(),
				UniqueType<typename std::decay<I>::type>::Get(),
				UniqueType<typename std::decay<J>::type>::Get(),
				UniqueType<typename std::decay<K>::type>::Get(),
				UniqueType<typename std::decay<L>::type>::Get(),
				UniqueType<typename std::decay<M>::type>::Get(),
				UniqueType<typename std::decay<N>::type>::Get(),
				UniqueType<typename std::decay<O>::type>::Get(),
				UniqueType<typename std::decay<P>::type>::Get()
			};

			static bool isReferences[] = {
				std::is_reference<A>::value,
				std::is_reference<B>::value,
				std::is_reference<C>::value,
				std::is_reference<D>::value,
				std::is_reference<E>::value,
				std::is_reference<F>::value,
				std::is_reference<G>::value,
				std::is_reference<H>::value,
				std::is_reference<I>::value,
				std::is_reference<J>::value,
				std::is_reference<K>::value,
				std::is_reference<L>::value,
				std::is_reference<M>::value,
				std::is_reference<N>::value,
				std::is_reference<O>::value,
				std::is_reference<P>::value,
			};

			static bool isConsts[] = {
				std::is_const<A>::value,
				std::is_const<B>::value,
				std::is_const<C>::value,
				std::is_const<D>::value,
				std::is_const<E>::value,
				std::is_const<F>::value,
				std::is_const<G>::value,
				std::is_const<H>::value,
				std::is_const<I>::value,
				std::is_const<J>::value,
				std::is_const<K>::value,
				std::is_const<L>::value,
				std::is_const<M>::value,
				std::is_const<N>::value,
				std::is_const<O>::value,
				std::is_const<P>::value,
			};

			static Param retValue(UniqueType<R>::Get(), UniqueType<typename std::decay<R>::type>::Get(), std::is_reference<R>::value, std::is_const<R>::value);
			std::vector<Param> p;
			for (size_t i = 0; i < sizeof(params) / sizeof(params[0]) && (!(params[i] == UniqueType<Void>::Get())); i++) {
				p.emplace_back(Param(params[i], decayParams[i], isReferences[i], isConsts[i]));
			}

			Method(name, reinterpret_cast<const TProxy<>*>(&t.GetProxy()), retValue, p, meta);
		}
#else
		// For method reflection
		template <typename R, typename... Args>
		inline void OnMethod(const TWrapper<R, Args...>& t, const char* name, const MetaChainBase* meta) {
			std::vector<Param> params;
			ParseParams(params, t);
			Unique u = UniqueType<R>::Get();
			Unique d = UniqueType<typename std::decay<R>::type>::Get();
			Param retValue(u, d, std::is_reference<R>::value, std::is_const<typename std::remove_reference<R>::type>::value);
			Method(name, reinterpret_cast<const TProxy<>*>(&t.GetProxy()), retValue, params, meta);
		}

		template <typename R, typename V, typename... Args>
		inline void ParseParams(std::vector<Param>& params, const TWrapper<R, V, Args...>&) {
			Unique u = UniqueType<V>::Get();
			Unique d = UniqueType<typename std::decay<V>::type>::Get();
			params.emplace_back(Param(u, d, std::is_reference<V>::value, std::is_const<typename std::remove_reference<V>::type>::value));
			ParseParams(params, TWrapper<R, Args...>());
		}

		template <typename R>
		inline void ParseParams(std::vector<Param>& params, const TWrapper<R>&) {}
#endif

		// override these functions and enable isReflectXXXXX to accept corresponding reflection requests
		virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);
		virtual void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta);
		virtual void Class(IReflectObject& host, Unique id, const char* name, const char* path, const MetaChainBase* meta);
		virtual void Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta);

	private:
		bool isReflectProperty;
		bool isReflectMethod;
		bool isReflectClass;
		bool isReflectEnum;
	};

	// Here are some sample Meta classes
	// Note is just a note.
	class MetaNote : public MetaNodeBase {
	public:
		MetaNote(const String& v);
		MetaNote operator = (const String& value);

		template <class T, class D>
		inline const MetaNote& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaNote Type;
		};

		typedef MetaNote Type;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		String value;
	};

	extern MetaNote Note;

	class MetaParameter : public MetaNodeBase {
	public:
		MetaParameter(const String& v, void* proto = nullptr);
		MetaParameter operator = (const String& value);

		template <class T, class D>
		inline const MetaParameter& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaParameter Type;
		};

		typedef MetaParameter Type;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		String value;
		void* prototype;
	};

	extern MetaParameter Parameter;

	template <class T, class D>
	class WriterBase : public MetaChainBase {
	public:
		WriterBase(IReflect& r, T* p, D* v, const char* n) : reflect(r), object(p), member(v), name(n) {}
		~WriterBase() override {}

		inline bool IsChain() const { return false; }

		virtual void Finish(const MetaChainBase* head) {}

#if defined(_MSC_VER) && _MSC_VER <= 1200
		template <class N>
		inline MetaChain<T, D, N::Type, WriterBase<T, D> > operator [] (const N& t) const {
			return MetaChain<T, D, N::Type, WriterBase<T, D> >(t.FilterField(object, member), *this);
		}

#else
		template <class N>
		inline MetaChain<T, D, typename N::template RealType<T, D>::Type, WriterBase<T, D> > operator [] (const N& t) const {
			return MetaChain<T, D, typename N::template RealType<T, D>::Type, WriterBase<T, D> >(t.FilterField(object, member), *this);
		}

#endif

		inline T* GetObject() const {
			return object;
		}

		inline D* GetMember() const {
			return member;
		}

		IReflect& reflect;
		T* object;
		D* member;
		const char* name;
	};

	template <class T>
	struct VectorIteratorHelper {
		template <class D>
		static VectorIterator<D> Transform(D* d) {
			return VectorIterator<D>(d);
		}
	};

	template <>
	struct VectorIteratorHelper<std::false_type> {
		template <class D>
		static D& Transform(D* d) {
			return *d;
		}
	};

	// Meta for properties auto conversion (e.g. std::vector<>)
	template <class T, class D>
	class PropertyWriter : public WriterBase<T, D> {
	public:
		PropertyWriter(IReflect& r, T* p, D* v, const char* n) : WriterBase<T, D>(r, p, v, n) {}
		~PropertyWriter() override {}

		void operator * () {
			Finish(nullptr);
		}

		void Finish(const MetaChainBase* head) override {
#if defined(_MSC_VER) && _MSC_VER <= 1200
			WriterBase<T, D>::reflect.OnProperty(VectorIteratorHelper<std::is_vector<std::decay<D>::type>::type>::Transform(WriterBase<T, D>::member), WriterBase<T, D>::name, WriterBase<T, D>::object, WriterBase<T, D>::member, head == this ? nullptr : head);
#else
			WriterBase<T, D>::reflect.OnProperty(VectorIteratorHelper<typename std::is_vector<typename std::decay<D>::type>::type>::Transform(WriterBase<T, D>::member), WriterBase<T, D>::name, WriterBase<T, D>::object, WriterBase<T, D>::member, head == this ? nullptr : head);
#endif
		}
	};

	template <class T, class D>
	PropertyWriter<T, typename std::remove_shared<D>::type> CreatePropertyWriter(IReflect& r, T* p, const D& v, const char* name) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
		return PropertyWriter<T, std::remove_shared<D>::type>(r, p, (std::remove_shared<D>::type*)const_cast<D*>(&v), name);
#else
		return PropertyWriter<T, typename std::remove_shared<D>::type>(r, p, (typename std::remove_shared<D>::type*)const_cast<D*>(&v), name);
#endif
	}

	template <class T, class D>
	class MethodWriter : public WriterBase<T, D> {
	public:
		MethodWriter(IReflect& r, T* p, D* v, const char* n) : WriterBase<T, D>(r, p, v, n) {}
		~MethodWriter() override {}

		void operator * () {
			Finish(nullptr);
		}

		void Finish(const MetaChainBase* head) override {
			WriterBase<T, D>::reflect.OnMethod(Wrap(WriterBase<T, D>::object, *WriterBase<T, D>::member), WriterBase<T, D>::name, head == this ? nullptr : head);
		}
	};

	template <class T, class D>
	MethodWriter<T, D> CreateMethodWriter(IReflect& r, T* p, const D& v, const char* name) {
		return MethodWriter<T, D>(r, p, const_cast<D*>(&v), name);
	}

	// Meta for class writer
	template <class T>
	class ClassWriter : public WriterBase<T, const char>  {
	public:
		ClassWriter(IReflect& r, T* p, const char* l) : WriterBase<T, const char>(r, p, l, "Class") {}
		~ClassWriter() override {}

		void operator * () {
			Finish(nullptr);
		}

		void Finish(const MetaChainBase* head) override {
			WriterBase<T, const char>::reflect.OnClass(*WriterBase<T, const char>::object, UniqueType<T>::Get()->GetName().c_str(), WriterBase<T, const char>::member, head);
		}
	};

	template <class T>
	ClassWriter<T> CreateClassWriter(IReflect& r, T* v, const char* line) {
		return ClassWriter<T>(r, v, line);
	}

	// Meta for enum writer
	template <class T>
	class EnumWriter : public WriterBase<T, T>  {
	public:
		EnumWriter(IReflect& r, T v, const char* n) : value(v), WriterBase<T, T>(r, &value, &value, n) {}
		~EnumWriter() override {}

		void operator * () {
			Finish(nullptr);
		}

		void Finish(const MetaChainBase* head) override {
			WriterBase<T, T>::reflect.OnEnum(value, WriterBase<T, T>::name, head);
		}

	private:
		T value;
	};

	template <class T>
	EnumWriter<T> CreateEnumWriter(IReflect& r, T v, const char* name) {
		return EnumWriter<T>(r, v, name);
	}

	template <class T>
	class Creatable {
	public:
		Creatable() {
			singleton Unique u = UniqueType<T>::Get();
			u->creator = &Creatable::Create;
		}
		static void Init() {
			static Creatable<T> theInstance;
		}
		static IReflectObject* Create() {
			return new T();
		}
	};

	// Meta for constructible (i.e. can be created from Unique::Create)
	class MetaConstructable : public MetaNodeBase {
	public:
		template <class T, class D>
		inline const MetaConstructable& FilterField(T* t, D* d) const {
			static Creatable<T> theClass;
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaConstructable Type;
		};

		typedef MetaConstructable Type;
		Unique GetUnique() const override;
	};

	extern MetaConstructable Constructable;

	template <class T>
	class Copyable {
	public:
		Copyable() {
			singleton Unique u = UniqueType<T>::Get();
			u->cloner = &Copyable::Copy;
		}

		static void Init() {
			static Copyable<T> theInstance;
		}

		static IReflectObject* Copy(const IReflectObject& src) {
			return new T(static_cast<const T&>(src));
		}
	};

	// Meta for cloneable (i.e. can be created from Unique::Create)
	class MetaCloneable : public MetaNodeBase {
	public:
		template <class T, class D>
		inline const MetaCloneable& FilterField(T* t, D* d) const {
			static Copyable<T> theClass;
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaCloneable Type;
		};

		typedef MetaCloneable Type;
		Unique GetUnique() const override;
	};

	extern MetaCloneable Cloneable;

	// Meta for register derivation relationships
	template <class T, class P>
	class RegisterInterface {
	public:
		RegisterInterface() {
			singleton Unique t = UniqueType<T>::Get();
			singleton Unique p = UniqueType<P>::Get();

			// We do not support virtual inheritance
			// Just gen offset from T to P
			const T* object = (const T*)0x10000;
			const P* convert = static_cast<const P*>(object);
			size_t offset = (uint8_t*)convert - (uint8_t*)object;

			assert(t->critical.exchange(1, std::memory_order_acquire) == 0);
			t->interfaces.reserve(t->interfaces.size() + p->interfaces.size() + 1);
			BinaryInsert(t->interfaces, MakeKeyValue(p.GetInfo(), offset));
			// merge casts
			for (size_t k = 0; k < p->interfaces.size(); k++) {
				KeyValue<UniqueInfo*, size_t> v = p->interfaces[k];
				BinaryInsert(t->interfaces, MakeKeyValue(v.first, v.second + offset));
			}
			assert(t->critical.exchange(0, std::memory_order_acquire) == 1);
		}
	};

	class IUniversalInterface {
	public:
 		// Make MetaInterface happy.
		TObject<IReflect>& operator () (IReflect& reflect) {
			static IReflectObject dummy;
			return dummy;
		}
	};

	// Meta for adding interface
	template <class P>
	class MetaInterface : public MetaNodeBase {
	public:
		P& object;
		class DummyReflect : public IReflect {
		public:
			DummyReflect() : IReflect(false, false) {}
		};

		MetaInterface(P& obj) : object(obj) {
			static DummyReflect dummyReflect;
			object.P::operator () (dummyReflect);
		}

		template <class T, class D>
		inline const MetaInterface<P>& FilterField(T* t, D* d) const {
			// class T is Interfaced form class P
			TSingleton<RegisterInterface<T, P> >::Get();
			
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaInterface<P> Type;
		};

		typedef MetaInterface<P> Type;

		TObject<IReflect>& operator () (IReflect& reflect) override {
			return object.P::operator () (reflect);
		}
	};

	template <class T>
	class BuiltinTypeWrapper : public IReflectObject {};

#define ReflectBuiltinSubtype(v) \
	{ BuiltinTypeWrapper<v> proto; reflect.Class(proto, UniqueType<v>::Get(), #v, "C++", nullptr); }

#define ReflectBuiltinType(v) \
	ReflectBuiltinSubtype(v); \
	ReflectBuiltinSubtype(const v); \
	ReflectBuiltinSubtype(v&); \
	ReflectBuiltinSubtype(const v&); \
	ReflectBuiltinSubtype(v*); \
	ReflectBuiltinSubtype(const v*); \
	ReflectBuiltinSubtype(v*&); \
	ReflectBuiltinSubtype(const v*&); \

#define ReflectInterface(c) \
	MetaInterface<c>(*this)

#define ReflectClassImpl(v) \
	CreateClassWriter(reflect, static_cast<v*>(this), __FILE__)

#define ReflectClass(v) \
	*ReflectClassImpl(v)

#define ReflectEnum(v) \
	*CreateEnumWriter(reflect, v, #v);

#define ReflectProperty(v) \
	*CreatePropertyWriter(reflect, this, v, #v)

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define ReflectMethod(v) \
	*CreateMethodWriter(reflect, this, &Class::v, #v)
#else
#define ReflectMethod(v) \
	*CreateMethodWriter(reflect, this, &std::remove_reference<decltype(*this)>::type::v, #v)
#endif

	class Inspector : public IReflect {
	public:
		Inspector(const IReflectObject& r);

		template <class T>
		T* operator [](UniqueType<T> ut) {
			singleton Unique u = UniqueType<T>::Get();
			return reinterpret_cast<T*>(Find(u, ut));
		}

		virtual void* Find(Unique unique, const String& key) const;
		void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
		void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override;

	private:
		std::map<Unique, std::map<String, void*> > entries;
	};

	class MetaRuntime : public MetaNodeBase {
	public:
		template <class T, class D>
		inline const MetaRuntime& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaRuntime Type;
		};

		typedef MetaRuntime Type;
		TObject<IReflect>& operator () (IReflect& reflect) override;
	};

	extern MetaRuntime Runtime;

	class MetaVoid : public MetaNodeBase {
	public:
		template <class T, class D>
		inline const MetaVoid& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaVoid Type;
		};

		typedef MetaVoid Type;
		Unique GetUnique() const override;
	};

	// Helper class for implement rtti by simple inheriance.
	template <class T, class Base, class MetaOption0 = MetaVoid, class MetaOption1 = MetaVoid>
	class pure_interface TReflected : public Base {
	protected:
#if defined(_MSC_VER) && _MSC_VER <= 1200
		TReflected() {}
		template <class A>
		TReflected(A& a) : Base(a) {}
		template <class A, class B>
		TReflected(A& a, B& b) : Base(a, b) {}
		template <class A, class B, class C>
		TReflected(A& a, B& b, C& c) : Base(a, b, c) {}
		template <class A, class B, class C, class D>
		TReflected(A& a, B& b, C& c, D& d) : Base(a, b, c, d) {}
		template <class A, class B, class C, class D, class E>
		TReflected(A& a, B& b, C& c, D& d, E& e) : Base(a, b, c, d, e) {}
		template <class A, class B, class C, class D, class E, class F>
		TReflected(A& a, B& b, C& c, D& d, E& e, F& f) : Base(a, b, c, d, e, f) {}
		template <class A, class B, class C, class D, class E, class F, class G>
		TReflected(A& a, B& b, C& c, D& d, E& e, F& f, G& g) : Base(a, b, c, d, e, f, g) {}
		template <class A, class B, class C, class D, class E, class F, class G, class H>
		TReflected(A& a, B& b, C& c, D& d, E& e, F& f, G& g, H& h) : Base(a, b, c, d, e, f, g, h) {}
#else
		template <typename... Args>
		TReflected(Args&&... args) : Base(std::forward<Args>(args)...) {}
#endif
	public:
		typedef T Class;
		typedef TReflected BaseClass;
		typedef Base NativeBaseClass;

		TObject<IReflect>& operator () (IReflect& reflect) override {
			singleton MetaOption0 option0;
			singleton MetaOption1 option1;
			ReflectClass(Class)[ReflectInterface(NativeBaseClass)][option0][option1];
			return *this;
		}

		Unique GetUnique() const override {
			singleton Unique unique = IReflectObject::GetUnique();
			return unique;
		}

		IReflectObject* Clone() const override {
			singleton Unique unique = IReflectObject::GetUnique();
			return unique->IsCloneable() ? unique->cloner(*this) : nullptr;
		}
	};
}

#if !defined(_MSC_VER) || _MSC_VER > 1200
namespace std {
#endif
	template <>
	struct hash<PaintsNow::Unique> {
		size_t operator () (const PaintsNow::Unique& unique) const {
			return (size_t)unique.GetInfo();
		}
	};
#if !defined(_MSC_VER) || _MSC_VER > 1200
}

#endif
