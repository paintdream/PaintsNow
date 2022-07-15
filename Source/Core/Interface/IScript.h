// IScript.h
// PaintDream (paintdream@paintdream.com)
// 2013-7-24
//

#pragma once
#include "../PaintsNow.h"
#include "IType.h"
#include "IReflect.h"
#include "IThread.h"
#include "../Template/TVector.h"
#include "../Template/TAtomic.h"
#include "../Template/TProxy.h"
#include "../Template/TMap.h"
#include "../Template/TAlgorithm.h"
#include "../Template/TPool.h"
#include "../Template/TBuffer.h"
#include <vector>
#include <list>
#include <stack>
#include <map>
#include <string>
#include <cassert>
#include <utility>

#ifdef _MSC_VER
#pragma warning(disable:4263)
#endif

#ifdef _DEBUG
#define DEBUG_SCRIPT
#endif

namespace PaintsNow {
	class pure_interface IScript : public IDevice, public ISyncObject {
	public:
		class Request;

		class Object : public TReflected<Object, IReflectObjectComplex> {
		public:
			Object();
			~Object() override;

			virtual void ScriptInitialize(Request& request);
			virtual void ScriptUninitialize(Request& request);
		};

		template <class T>
		class TManagedReference {
		public:
			TManagedReference(T* t = nullptr) : instance(t) {}

			operator bool () const {
				return instance != nullptr;
			}

			T& GetInstance() const { return *instance; }

			void SetInstance(T& t) const {
				if (instance == nullptr) {
					const_cast<T*&>(instance) = &t;
				} else {
					*instance = t;
				}
			}

			operator T& () {
				return *instance;
			}

			operator const T& () const {
				return *instance;
			}

		private:
			T* instance;
		};

		template <class T>
		static TManagedReference<T> ManageReference(const T& t) {
			return TManagedReference<T>(const_cast<T*>(&t));
		}

		class ManagedObject : public TReflected<ManagedObject, IReflectObjectComplex> {
		public:
			virtual void* GetInstanceData() = 0;
			virtual const void* GetInstanceData() const = 0;
			virtual size_t GetInstanceLength() const = 0;
			virtual void Clone(void* p) const = 0;
			virtual size_t GetLength() const = 0;
		};

		template <class T>
		class TManagedObject : public TReflected<TManagedObject<T>, ManagedObject> {
		public:
			TManagedObject(const T& rhs) : instance(rhs) {}
			TManagedObject(rvalue<T> rhs) : instance(std::move(rhs)) {}

			operator T& () {
				return instance;
			}

			operator const T& () const {
				return instance;
			}

			operator TManagedReference<T>() const {
				return Ref();
			}

			void* GetInstanceData() override { return &instance; }
			const void* GetInstanceData() const override { return &instance; }
			size_t GetInstanceLength() const override { return sizeof(T); }
			void Clone(void* p) const override { new (p) TManagedObject<T>(instance); }
			size_t GetLength() const override { return sizeof(*this); }

			T& GetInstance() { return instance; }
			const T& GetInstance() const { return instance; }
			size_t ReportMemoryUsage() const override { return sizeof(*this); }
			TManagedReference<T> Ref() const { return TManagedReference<T>(const_cast<T*>(&instance)); }

		private:
			T instance;
		};

		template <class T>
		static TManagedObject<T> ManageObject(const T& t) {
			return TManagedObject<T>(t);
		}

		class Library : public Object {
		public:
			Library();
			~Library() override;
			virtual void TickDevice(IDevice& device);
			virtual void Initialize();
			virtual void Uninitialize();
			virtual void ScriptRequire(Request& request);
			void ScriptInitialize(Request& request) override;
			void ScriptUninitialize(Request& request) override;

			void Register(Request& request);
		};

		class BaseDelegate {
		public:
			BaseDelegate(Object* p = nullptr) : ptr(p) {}
			Object* Get() const { return ptr; }

		protected:
			Object* ptr;
		};

		template <class T>
		class Delegate : public BaseDelegate {
		public:
			inline const String& GetTypeName() const {
				singleton Unique u = UniqueType<T>::Get();
				return u->GetName();
			}

			inline T* Get() const {
				singleton Unique u = UniqueType<T>::Get();
				Object* p = ptr;
				if (p != nullptr && (u == p->GetUnique() || p->QueryInterface(UniqueType<T>()) != nullptr)) {
					return static_cast<T*>(p);
				} else {
					return nullptr;
				}
			}

			inline T* operator -> () const {
				return Get();
			}

			inline operator bool() const {
				return Get() != nullptr;
			}
		};

		// thread support for requests
		IScript(IThread& api);
		~IScript() override;

		class RequestPool;
		class Request : public TReflected<Request, IReflectObjectComplex> {
		protected:
			RequestPool* requestPool;
		public:
			Request* next; // for request pooling

		public:
			Request();
			~Request() override;
			virtual void DoLock();
			virtual void UnLock();
			virtual bool TryLock();

			virtual RequestPool* GetRequestPool();
			virtual void SetRequestPool(RequestPool* pool);
			virtual void* GetNativeScript();

			// For variadic arguments
			class Arguments {
			public:
				Arguments() : count(0) {}
				size_t count;
			};

			// Script internal types
			enum TYPE { NIL, BOOLEAN, NUMBER, INTEGER, STRING, TABLE, ARRAY, FUNCTION, OBJECT, ANY };

			// Reference handler for script objects
			class Ref {
			public:
				Ref(size_t i = 0) : value(i)
#ifdef _DEBUG
				, hostScript(nullptr)
#endif
				{}

				operator bool () const {
					return value != 0;
				}

				size_t value;
#ifdef _DEBUG
				IScript* hostScript;
#endif
			};

			// Callable objects
			class AutoWrapperBase {
			public:
				virtual ~AutoWrapperBase() {}
				virtual bool IsSync() const;
				virtual void Execute(Request& request) const = 0;
				virtual AutoWrapperBase* Clone() const = 0;
			};

			// Call signature
			class Sync : public AutoWrapperBase {
			public:
				bool IsSync() const override;
				void Execute(Request& request) const override;
				AutoWrapperBase* Clone() const override;
			};

#if (defined(_MSC_VER) && _MSC_VER < 1800)
			template <bool LockOnCall, class R = Void, class A = Void, class B = Void, class C = Void, class D = Void, class E = Void, class F = Void, class G = Void, class H = Void, class I = Void, class J = Void, class K = Void, class L = Void, class M = Void, class N = Void, class O = Void>
			class AutoWrapper : public AutoWrapperBase, public TWrapper<R, Request&, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> {
			public:
				AutoWrapper(const TWrapper<R, Request&, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>& m) : TWrapper<R, Request&, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(m) {}

				virtual AutoWrapperBase* Clone() const {
					return new AutoWrapper(*this);
				}

				void InvokeRoutine(Request& request) {
					std::decay<A>::type a; std::decay<B>::type b; std::decay<C>::type c; std::decay<D>::type d; std::decay<E>::type e;
					std::decay<F>::type f; std::decay<G>::type g; std::decay<H>::type h; std::decay<I>::type i; std::decay<J>::type j;
					std::decay<K>::type k; std::decay<L>::type l; std::decay<M>::type m; std::decay<N>::type n; std::decay<O>::type o;

					std::decay<ReturnType<R>::type>::type ret;
				//	request.AssertUnlocked();
					switch (GetCount()) {
					case 0:
						assert(false);
						break;
					case 1:
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request);
						if (!LockOnCall) request.DoLock();
						break;
					case 2:
						request >> a;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a);
						if (!LockOnCall) request.DoLock();
						break;
					case 3:
						request >> a >> b;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b);
						if (!LockOnCall) request.DoLock();
						break;
					case 4:
						request >> a >> b >> c;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c);
						if (!LockOnCall) request.DoLock();
						break;
					case 5:
						request >> a >> b >> c >> d;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d);
						if (!LockOnCall) request.DoLock();
						break;
					case 6:
						request >> a >> b >> c >> d >> e;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e);
						if (!LockOnCall) request.DoLock();
						break;
					case 7:
						request >> a >> b >> c >> d >> e >> f;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f);
						if (!LockOnCall) request.DoLock();
						break;
					case 8:
						request >> a >> b >> c >> d >> e >> f >> g;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g);
						if (!LockOnCall) request.DoLock();
						break;
					case 9:
						request >> a >> b >> c >> d >> e >> f >> g >> h;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h);
						if (!LockOnCall) request.DoLock();
						break;
					case 10:
						request >> a >> b >> c >> d >> e >> f >> g >> h >> i;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h, i);
						if (!LockOnCall) request.DoLock();
						break;
					case 11:
						request >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h, i, j);
						if (!LockOnCall) request.DoLock();
						break;
					case 12:
						request >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h, i, j, k);
						if (!LockOnCall) request.DoLock();
						break;
					case 13:
						request >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h, i, j, k, l);
						if (!LockOnCall) request.DoLock();
						break;
					case 14:
						request >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l >> m;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h, i, j, k, l, m);
						if (!LockOnCall) request.DoLock();
						break;
					case 15:
						request >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l >> m >> n;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h, i, j, k, l, m, n);
						if (!LockOnCall) request.DoLock();
						break;
					case 16:
						request >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l >> m >> n >> o;
						if (!LockOnCall) request.UnLock();
						ret = (*this)(request, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
						if (!LockOnCall) request.DoLock();
						break;
					default:
						break;
					}

					request << ret;
				//	request.AssertUnlocked();
				}

				virtual void Execute(Request& request) const {
					const TWrapper<void, Request&, IHost*, size_t, const TWrapper<void, Request&>& >& dispatcher = request.GetScript()->GetDispatcher();
					if (dispatcher) {
						dispatcher(request, GetHost(), GetCount(), Wrap(const_cast<AutoWrapper*>(this), &AutoWrapper::InvokeRoutine));
					} else {
						(const_cast<AutoWrapper*>(this))->InvokeRoutine(request);
					}
				}
			};

			template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
			static AutoWrapper<false, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> Adapt(const TWrapper<R, Request&, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>& wrapper) {
				return AutoWrapper<false, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(wrapper);
			}

			template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
			static AutoWrapper<true, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> AdaptLocked(const TWrapper<R, Request&, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>& wrapper) {
				return AutoWrapper<true, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(wrapper);
			}

			template <class A>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a) {
				*this << a;
				return Call(defer, ref);	
			}

			template <class A>
			bool Call(const IScript::Request::Ref& ref, const A& a) {
				*this << a;
				return Call(Sync(), ref);	
			}

			template <class A, class B>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b) {
				*this << a << b;
				return Call(defer, ref);	
			}

			template <class A, class B>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b) {
				*this << a << b;
				return Call(Sync(), ref);	
			}

			template <class A, class B, class C>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c) {
				*this << a << b << c;
				return Call(defer, ref);	
			}

			template <class A, class B, class C>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c) {
				*this << a << b << c;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d) {
				*this << a << b << c << d;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d) {
				*this << a << b << c << d;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
				*this << a << b << c << d << e;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
				*this << a << b << c << d << e;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
				*this << a << b << c << d << e << f;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
				*this << a << b << c << d << e << f;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
				*this << a << b << c << d << e << f << g;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
				*this << a << b << c << d << e << f << g;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h) {
				*this << a << b << c << d << e << f << g << h;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h) {
				*this << a << b << c << d << e << f << g << h;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i) {
				*this << a << b << c << d << e << f << g << h << i;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i) {
				*this << a << b << c << d << e << f << g << h << i;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j) {
				*this << a << b << c << d << e << f << g << h << i << j;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j) {
				*this << a << b << c << d << e << f << g << h << i << j;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k) {
				*this << a << b << c << d << e << f << g << h << i << j << k;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k) {
				*this << a << b << c << d << e << f << g << h << i << j << k;
				return Call(Sync, ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m << n;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m << n;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m << n << o;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m << n << o;
				return Call(Sync(), ref);
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class AA>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o, const AA& aa) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m << n << o << aa;
				return Call(defer, ref);	
			}

			template <class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class AA>
			bool Call(const IScript::Request::Ref& ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g, const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o, const AA& aa) {
				*this << a << b << c << d << e << f << g << h << i << j << k << l << m << n << o << aa;
				return Call(Sync(), ref);
			}

#else
			template <bool LockOnCall, typename R, typename... Args>
			class AutoWrapper : public AutoWrapperBase, public TWrapper<R, Request&, Args...> {
			public:
				AutoWrapper(const TWrapper<R, Request&, Args...>& m) : TWrapper<R, Request&, Args...>(m) {}
				AutoWrapperBase* Clone() const override {
					return new AutoWrapper(*this);
				}

				template <typename Ret, typename T, size_t... I>
				typename std::enable_if<!std::is_void<Ret>::value>::type Apply(Request& request, T& arg, seq<I...>) const {
					// request.AssertUnlocked();
					Ret ret = (*this)(request, std::try_move<typename std::tuple_element<I, std::tuple<Args...>>::type>(std::get<I>(arg))...);
					if (!LockOnCall) {
						request.DoLock();
					}

					request << ret;
					// request.AssertUnlocked();
				}

				template <typename Ret, typename T, size_t... I>
				typename std::enable_if<std::is_void<Ret>::value>::type Apply(Request& request, T& arg, seq<I...>) const {
					// request.AssertUnlocked();
					(*this)(request, std::try_move<typename std::tuple_element<I, std::tuple<Args...>>::type>(std::get<I>(arg))...);

					if (!LockOnCall) {
						request.DoLock();
					}
					// request.AssertUnlocked();
				}

				template <typename T, size_t index>
				struct Reader {
					void operator () (Request& request, T& arg) {
						request >> std::get<std::tuple_size<T>::value - index>(arg);
						Reader<T, index - 1>()(request, arg);
					}
				};

				template <typename T>
				struct Reader<T, 0> {
					void operator () (Request& request, T& arg) {}
				};

				void InvokeRoutine(Request& request) const {
					std::tuple<typename std::decay<Args>::type...> arg;
					Reader<decltype(arg), sizeof...(Args)>()(request, arg);
					if (!LockOnCall) {
						request.UnLock();
					}
					Apply<R>(request, arg, gen_seq<sizeof...(Args)>());
				}

				void Execute(Request& request) const override {
					IScript* script = request.GetScript();
					if (script != nullptr) { // hook
						const TWrapper<void, Request&, IHost*, size_t, const TWrapper<void, Request&>& >& dispatcher = script->GetDispatcher();
						if (dispatcher) {
							dispatcher(request, TWrapper<R, Request&, Args...>::GetHost(), TWrapper<R, Request&, Args...>::GetCount(), Wrap(this, &AutoWrapper::InvokeRoutine));
							return;
						}
					}

					InvokeRoutine(request);
				}
			};

			template <typename R, typename... Args>
			static AutoWrapper<false, R, Args...> Adapt(const TWrapper<R, Request&, Args...>& wrapper) {
				return AutoWrapper<false, R, Args...>(wrapper);
			}

			template <typename R, typename... Args>
			static AutoWrapper<true, R, Args...> AdaptLocked(const TWrapper<R, Request&, Args...>& wrapper) {
				return AutoWrapper<true, R, Args...>(wrapper);
			}

			template <typename First, typename... Args>
			bool Call(const AutoWrapperBase& defer, const IScript::Request::Ref& ref, const First& first, Args&&... args) {
				*this << first;
				return Call(defer, ref, std::forward<Args>(args)...);
			}

			template <typename First, typename... Args>
			bool Call(const IScript::Request::Ref& ref, const First& first, Args&&... args) {
				*this << first;
				return Call(Sync(), ref, std::forward<Args>(args)...);
			}
#endif

			inline bool Call(const Request::Ref& ref) {
				return Call(Sync(), ref);
			}

			// Mark types
			struct Nil {};
			struct Global {};
			struct ArrayStart { size_t count; };
			struct ArrayEnd {};
			struct TableStart { size_t count; };
			struct TableEnd {};

			template <class T>
			class VarKey {
			public:
				VarKey(const T* object) : binding(object) {}

				const T* binding;
			};

			class Key {
			public:
				template <class T>
				VarKey<T> operator () (const T& object) const { return VarKey<T>(&object); }
				VarKey<const char> operator () (const char* object) const { return VarKey<const char>(object); }
			};

			class Iterator {
			public:
				Iterator() : keyType(NIL), valueType(NIL) {}
				operator bool() const { return valueType != NIL; }
			
				TYPE keyType;
				TYPE valueType;
			};

			// Interfaces
			virtual int GetCount() = 0;
			virtual IScript* GetScript() = 0;
			virtual void QueryInterface(const TWrapper<void, Request&, IReflectObject&, const Ref&>& callback, IReflectObject& target, const Ref& g);
			virtual bool Call(const AutoWrapperBase& defer, const Request::Ref& g) = 0;
			virtual TYPE GetCurrentType() = 0;
			virtual Request::Ref Load(const String& script, const String& pathname = String()) = 0;
			virtual Request& Push() = 0;
			virtual Request& Pop() = 0;
			virtual Ref Reference(const Ref& d) = 0;
			virtual TYPE GetReferenceType(const Ref& d) = 0;
			virtual void Dereference(Ref& ref) = 0;
			virtual AutoWrapperBase* GetWrapper(const Ref& r) = 0;
			virtual void Error(const String& msg);
			virtual Request& MoveVariables(Request& target, size_t count) = 0;

			// Special nodes
			virtual Request& operator >> (Arguments& ph) = 0;
			virtual Request& operator >> (Ref&) = 0;
			virtual Request& operator << (const Ref&) = 0;
			virtual Request& operator << (const Nil&) = 0;
			virtual Request& operator << (const BaseDelegate&) = 0;
			virtual Request& operator >> (BaseDelegate&) = 0;
			virtual Request& operator << (const Global&) = 0;
			virtual Request& operator << (const TableStart&) = 0;
			virtual Request& operator >> (TableStart&) = 0;
			virtual Request& operator << (const TableEnd&) = 0;
			virtual Request& operator << (const ArrayStart&) = 0;
			virtual Request& operator >> (ArrayStart&) = 0;
			virtual Request& operator << (const ArrayEnd&) = 0;
			virtual Request& operator << (const Key&) = 0;
			virtual Request& operator >> (Iterator&) = 0;
			virtual Request& operator << (const AutoWrapperBase& wrapper) = 0;

			virtual Request& operator << (Library& module);
			virtual Request& operator >> (IReflectObjectComplex& reflectObject);
			virtual Request& operator << (const IReflectObjectComplex& reflectObject);
			virtual Request& operator << (Unique str);
			virtual Request& operator >> (Unique& str);

			// Basic types
			virtual Request& operator << (double value) = 0;
			virtual Request& operator >> (double& value) = 0;
			virtual Request& operator << (float value);
			virtual Request& operator >> (float& value);
			virtual Request& operator << (const StringView& str) = 0;
			virtual Request& operator >> (StringView& str) = 0;
			virtual Request& operator << (const String& str) = 0;
			virtual Request& operator >> (String& str) = 0;
			virtual Request& operator << (const Bytes& bytes) = 0;
			virtual Request& operator >> (Bytes& bytes) = 0;
			virtual Request& operator << (const char* str) = 0;
			virtual Request& operator >> (const char*& str) = 0;
			virtual Request& operator << (bool value) = 0;
			virtual Request& operator >> (bool& value) = 0;
			virtual Request& operator << (int64_t u) = 0;
			virtual Request& operator >> (int64_t& u) = 0;

			virtual Request& operator >> (int8_t& t);
			virtual Request& operator << (int8_t t);
			virtual Request& operator >> (int16_t& t);
			virtual Request& operator << (int16_t t);
			virtual Request& operator >> (int32_t& t);
			virtual Request& operator << (int32_t t);
			virtual Request& operator >> (uint8_t& t);
			virtual Request& operator << (uint8_t t);
			virtual Request& operator >> (uint16_t& t);
			virtual Request& operator << (uint16_t t);
			virtual Request& operator >> (uint32_t& t);
			virtual Request& operator << (uint32_t t);

			virtual Request& operator >> (uint64_t& t);
			virtual Request& operator << (uint64_t t);
#ifdef _MSC_VER
			virtual Request& operator >> (long& t);
			virtual Request& operator << (long t);
			virtual Request& operator >> (unsigned long& t);
			virtual Request& operator << (unsigned long t);
#endif

			Request& operator >> (Void&);
			Request& operator << (const Void&);

		protected:
			virtual ManagedObject* WriteManaged(size_t size) = 0;
			virtual ManagedObject* ReadManaged() = 0;

		public:
			// Template types
			template <class T>
			Request& operator << (const verify_cast<T>& v) {
				return *this << (T)v;
			}

			ManagedObject* WriteClone(const ManagedObject& object) {
				ManagedObject* p = WriteManaged(object.GetLength());
				object.Clone(p);
				return p;
			}

			Request& operator << (const ManagedObject& object) {
				WriteClone(object);
				return *this;
			}

			Request& operator >> (ManagedObject*& object) {
				object = reinterpret_cast<ManagedObject*>(ReadManaged());
				return *this;
			}

			template <class T>
			Request& operator << (const TManagedReference<T>& v) {
				ManagedObject* p = WriteManaged(sizeof(TManagedObject<T>));
				new (p) TManagedObject<T>(v.GetInstance());

				return *this;
			}

			template <class T>
			Request& operator >> (const TManagedReference<T>& v) {
				ManagedObject* p = ReadManaged();
				if (p != nullptr) {
					assert(p->GetUnique() == UniqueType<TManagedObject<T> >::Get());
					v.SetInstance(static_cast<TManagedObject<T>*>(p)->GetInstance());
				}

				return *this;
			}

			template <class T>
			Request& operator << (const TManagedObject<T>& v) {
				return *this << (TManagedReference<T>)v;
			}

			template <class T>
			Request& operator >> (TManagedObject<T>& v) {
				return *this >> (TManagedReference<T>)v;
			}

			Request& operator << (Object* object) {
				return *this << IScript::BaseDelegate(object);
			}

			template <class T>
			Request& operator << (const VarKey<T>& varkey) {
				*this << Key();
				return *this << *varkey.binding;
			}

			Request& operator << (const VarKey<const char>& varkey) {
				*this << Key();
#if defined(_MSC_VER) && _MSC_VER <= 1200
				return *this << String(varkey.binding);
#else
				return *this << varkey.binding;
#endif
			}

			template <class T>
			Request& operator >> (std::vector<T>& vec) {
				Request& request = *this;
				ArrayStart ts;
				static ArrayEnd endarray;
				request >> ts;
				vec.resize(ts.count);
				for (size_t i = 0; i < ts.count; i++) {
					request >> vec[i];
				}
				request << endarray;
				return request;
			}

			template <class K, class V>
			Request& operator >> (std::map<K, V>& m) {
				Request& request = *this;
				TableStart begintable;
				TableEnd endtable;

				request >> begintable;
				Iterator it;
				while (true) {
					request >> it;
					if (!it) break;

					K key;
					V value;
					request >> key >> value;
					m.emplace(std::move(key), std::move(value));
				}

				request << endtable;
				return request;
			}

			template <class K, class V>
			Request& operator >> (std::unordered_map<K, V>& m) {
				Request& request = *this;
				TableStart begintable;
				TableEnd endtable;

				request >> begintable;
				Iterator it;
				while (true) {
					request >> it;
					if (!it) break;

					K key;
					V value;
					request >> key >> value;
					m.emplace(std::move(key), std::move(value));
				}
				request << endtable;
				return request;
			}

			template <class T>
			Request& operator >> (std::list<T>& vec) {
				Request& request = *this;
				ArrayStart ts;
				static ArrayEnd endarray;
				request >> ts;
				for (size_t i = 0; i < ts.count; i++) {
					vec.emplace_back(T());
					request >> vec.back();
				}
				request << endarray;
				return request;
			}

			template <class T, size_t n, size_t m>
			Request& operator >> (TMatrix<T, n, m>& mat) {
				Request& request = *this;
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				request >> beginarray;
				for (size_t j = 0; j < n; j++) {
					for (size_t i = 0; i < m; i++) {
						request >> mat(i, j);
					}
				}
				request << endarray;
				return request;
			}

			template <class T, size_t n>
			Request& operator >> (TVector<T, n>& vec) {
				Request& request = *this;
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				request >> beginarray;
				for (size_t i = 0; i < n; i++) {
					request >> vec[i];
				}
				request << endarray;
				return request;
			}

			template <class T, class D>
			Request& operator << (const std::pair<T, D>& pair) {
				Request& request = *this;
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				request << beginarray << pair.first << pair.second << endarray;
				return request;
			}

			template <class T, class D>
			Request& operator >> (std::pair<T, D>& pair) {
				Request& request = *this;
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				request >> beginarray >> pair.first >> pair.second << endarray;
				return request;
			}

			template <class T>
			Request& operator << (const std::vector<T>& vec) {
				Request& request = *this;
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				request << beginarray;
				for (size_t i = 0; i < vec.size(); i++) {
					request << vec[i];
				}
				request << endarray;
				return request;
			}

			template <class T>
			Request& operator << (const std::list<T>& vec) {
				Request& request = *this;
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				request << beginarray;
				for (typename std::list<T>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
					request << *it;
				}
				request << endarray;
				return request;
			}

			template <class K, class V>
			Request& operator << (const std::map<K, V>& m) {
				Request& request = *this;
				TableStart begintable;
				TableEnd endtable;

				request << begintable;
				for (typename std::map<K, V>::iterator it = m.begin(); it != m.end(); ++it) {
					request << key((*it).first) << (*it).second;
				}
				request << endtable;
				return request;
			}

			template <class K, class V>
			Request& operator << (const std::unordered_map<K, V>& m) {
				Request& request = *this;
				TableStart begintable;
				TableEnd endtable;

				request << begintable;
				for (typename std::unordered_map<K, V>::iterator it = m.begin(); it != m.end(); ++it) {
					request << key((*it).first) << (*it).second;
				}
				request << endtable;
				return request;
			}

			template <class T, size_t n, size_t m>
			Request& operator << (const TMatrix<T, n, m>& mat) {
				Request& request = *this;
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				request << beginarray;
				for (size_t j = 0; j < n; j++) {
					for (size_t i = 0; i < m; i++) {
						request << mat(i, j);
					}
				}
				request << endarray;
				return request;
			}

			template <class T, size_t n>
			Request& operator << (const TVector<T, n>& vec) {
				static ArrayStart beginarray;
				static ArrayEnd endarray;
				Request& request = *this;
				request << beginarray;
				for (size_t i = 0; i < n; i++) {
					request << vec[i];
				}
				request << endarray;
				return request;
			}
		};

		// Reflection meta for modules (libraries)
		class MetaLibrary : public TReflected<MetaLibrary, MetaNodeBase> {
		public:
			MetaLibrary(const String& name = "");
			MetaLibrary operator = (const String& name);

			template <class T, class D>
			inline const MetaLibrary& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef MetaLibrary Type;
			};

			typedef MetaLibrary Type;

			TObject<IReflect>& operator () (IReflect& reflect) override;
			const String& name;
		};

		// Reflection meta for methods
		class MetaMethod : public TReflected<MetaMethod, MetaNodeBase> {
		public:
			MetaMethod(const String& key = "", bool locked = false);
			~MetaMethod() override;
			MetaMethod operator = (const String& key);

			class TypedBase : public TReflected<TypedBase, MetaNodeBase> {
			public:
				virtual Request::AutoWrapperBase* CreateWrapper() const = 0;
				virtual Request& Register(Request& request, const String& defName) const = 0;

				String name;
			};

			// For vc6 compat
			class Type : public TReflected<Type, TypedBase> {
			public:
				Type(const Type& rhs) {
					name.~String();
					memcpy((void*)this, (const void*)&rhs, sizeof(*this));
					new (&name) String(rhs.name);
				}

				Request& Register(Request& request, const String& defName) const override {
					assert(false);
					return request;
				}

				Request::AutoWrapperBase* CreateWrapper() const override {
					assert(false);
					return nullptr;
				}

				void* pointer;
				void* member;
				bool lockOnCall;
			};

			template <class T, class D>
			class Typed : public TypedBase {
			public:
				Typed(const String& n, T* p, D* m, bool locked) : pointer(p), member(m), lockOnCall(locked) {
					name = n;
				}

				Request& Register(Request& request, const String& defName) const override {
					IScript::Request::Key key;
					if (lockOnCall) {
						request << key(name.empty() ? defName : name) << Request::AdaptLocked(Wrap(pointer, *member));
					} else {
						request << key(name.empty() ? defName : name) << Request::Adapt(Wrap(pointer, *member));
					}

					return request;
				}

				Request::AutoWrapperBase* CreateWrapper() const override {
					return Request::Adapt(Wrap(pointer, *member)).Clone();
				}

				T* pointer;
				D* member;
				bool lockOnCall;
			};

#if defined(_MSC_VER) && _MSC_VER <= 1200
			template <class T, class D>
			Type FilterField(T* pointer, D* member) const {
				Typed<T, D> typed(key, pointer, member, lockOnCall);
				return *(reinterpret_cast<Type*>(&typed));
			}
#else
			template <class T, class D>
			Typed<T, D> FilterField(T* pointer, D* member) const {
				return Typed<T, D>(key, pointer, member, lockOnCall);
			}
#endif

			template <class T, class D>
			struct RealType {
				typedef Typed<T, D> Type;
			};

			String key;
			bool lockOnCall;
		};

		// Reflection meta for variables
		class MetaVariable : public TReflected<MetaVariable, MetaNodeBase> {
		public:
			MetaVariable(const String& key = "");
			~MetaVariable() override;
			MetaVariable operator = (const String& key);

			class TypedBase : public TReflected<TypedBase, MetaNodeBase> {
			public:
				virtual Request& Read(Request& request, bool hasKey, const String& defName, void* overrideObject) = 0;
				virtual Request& Write(Request& request, bool hasKey, const String& defName, void* overrideObject) = 0;
				String name;
			};

			// For vc6 compat
			class Type : public TReflected<Type, TypedBase> {
			public:
				Type(const Type& rhs) {
					name = rhs.name;
				}

				IReflectObject* Clone() const override {
					assert(false);
					return nullptr;
				}
				Request& Read(Request& request, bool hasKey, const String& defName, void* overrideObject) override {
					assert(false);
					return request;
				}

				Request& Write(Request& request, bool hasKey, const String& defName, void* overrideObject) override {
					assert(false);
					return request;
				}

				void* object;
			};

			template <class T>
			class Typed : public TypedBase {
			public:
				Typed(const String& s, T* o = nullptr) : object(o) { name = s; }
				~Typed() override {}

				IReflectObject* Clone() const override {
					return new Typed(name, object);
				}
				Request& Read(Request& request, bool hasKey, const String& defName, void* overrideObject) override {
					if (hasKey) {
						request << Request::Key()(name.empty() ? defName : name);
					}

					request >> (overrideObject == nullptr ? *object : *reinterpret_cast<T*>(overrideObject));
					return request;
				}

				Request& Write(Request& request, bool hasKey, const String& defName, void* overrideObject) override {
					if (hasKey) {
						request << Request::Key()(name.empty() ? defName : name);
					}
					
					request << (overrideObject == nullptr ? *object : *reinterpret_cast<T*>(overrideObject));
					return request;
				}

				T* object;
			};
#if defined(_MSC_VER) && _MSC_VER <= 1200
			template <class T, class D>
			Type FilterField(T* pointer, D* member) const {
				Typed<D> typed(name, member);
				return *(reinterpret_cast<Type*>(&typed));
			}
#else
			template <class T, class D>
			inline Typed<D> FilterField(T* pointer, D* member) const {
				return Typed<D>(name, member);
			}
#endif

			template <class T, class D>
			struct RealType {
				typedef Typed<D> Type;
			};

			String name;
		};

		class MetaRemoteEntryBase : public TReflected<MetaRemoteEntryBase, MetaNodeBase> {
		public:
			MetaRemoteEntryBase() : callIndex(0) {}
			TWrapper<void> wrapper;
			String name;
			int callIndex;
		};

		// Reflection meta for remote entries
		template <class X>
		class MetaRemoteEntry : public MetaRemoteEntryBase {
		public:
			template <class T>
			static TWrapper<void> Convert(const T& t) {
				return *reinterpret_cast<const TWrapper<void>*>(&t);
			}

			MetaRemoteEntry operator = (const String& key) { name = key;  }

#if defined(_MSC_VER) && _MSC_VER <= 1200
			template <class A = Void, class B = Void, class C = Void, class D = Void, class E = Void, class F = Void, class G = Void, class H = Void, class I = Void, class J = Void, class K = Void, class L = Void, class M = Void>
			class Dispatcher : public TWrapper<std::pair<Request::Ref, size_t>, Request&, bool> {
			public:
				void Invoke0(const Request::AutoWrapperBase& wrapper, Request& request) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second);
					(*this)(request, false);
				}

				void Invoke1(const Request::AutoWrapperBase& wrapper, Request& request, A a) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a);
					(*this)(request, false);
				}

				void Invoke2(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b);
					(*this)(request, false);
				}

				void Invoke3(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c);
					(*this)(request, false);
				}

				void Invoke4(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d);
					(*this)(request, false);
				}

				void Invoke5(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e);
					(*this)(request, false);
				}

				void Invoke6(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f);
					(*this)(request, false);
				}

				void Invoke7(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g);
					(*this)(request, false);
				}

				void Invoke8(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g, H h) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g, h);
					(*this)(request, false);
				}

				void Invoke9(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g, H h, I i) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g, h, i);
					(*this)(request, false);
				}

				void Invoke10(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g, h, i, j);
					(*this)(request, false);
				}

				void Invoke11(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g, h, i, j, k);
					(*this)(request, false);
				}

				void Invoke12(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g, h, i, j, k, l);
					(*this)(request, false);
				}

				void Invoke13(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g, h, i, j, k, l);
					(*this)(request, false);
				}

				void Invoke14(const Request::AutoWrapperBase& wrapper, Request& request, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) {
					std::pair<Request::Ref, size_t> ref = (*this)(request, true);
					request.Call(wrapper, ref.first, ref.second, a, b, c, d, e, f, g, h, i, j, k, l, m);
					(*this)(request, false);
				}
			};

			template <class RR, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
			static TWrapper<void> Make(const TWrapper<RR, const Request::AutoWrapperBase&, Request&, A, B, C, D, E, F, G, H, I, J, K, L, M>& wp) {
				Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>* ptr = nullptr;
				switch (wp.GetCount() - 2) {
				case 0:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke0));
					break;
				case 1:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke1));
					break;
				case 2:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke2));
					break;
				case 3:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke3));
					break;
				case 4:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke4));
					break;
				case 5:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke5));
					break;
				case 6:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke6));
					break;
				case 7:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke7));
					break;
				case 8:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke8));
					break;
				case 9:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke9));
					break;
				case 10:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke10));
					break;
				case 11:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke11));
					break;
				case 12:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke12));
					break;
				case 13:
					return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke13));
					break;
				}

				assert(false);
				return Convert(Wrap(ptr, &Dispatcher<A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke0));
			}
#else
			template <typename... Args>
			class Dispatcher : public TWrapper<std::pair<Request::Ref, size_t>, Request&, bool> {
			public:
				void Invoke(const Request::AutoWrapperBase& wrapper, Request& request, Args&&... args) {
					std::pair<IScript::Request::Ref, size_t> r = (*this)(request, true);
					request.Call(wrapper, r.first, r.second, std::forward<Args>(args)...);
					(*this)(request, false);
				}
			};

			template <typename R, typename... Args>
			static TWrapper<void> Make(const TWrapper<R, const Request::AutoWrapperBase&, Request&, Args...>&) {
				return Convert(Wrap((Dispatcher<Args...>*)nullptr, &Dispatcher<Args...>::Invoke));
			}
#endif

			IReflectObject* Clone() const override {
				return new MetaRemoteEntry(*this);
			}

			template <class T>
			MetaRemoteEntry& FilterField(T* pointer, const X* member) const {
				const_cast<TWrapper<void>&>(wrapper) = Make(*member);
				return const_cast<MetaRemoteEntry&>(*this);
			}

			template <class T, class D>
			struct RealType {
				typedef MetaRemoteEntry Type;
			};

			typedef MetaRemoteEntry Type;
		};

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define __func__ "Unknown function at: " __FILE__
#endif

#define U ,
#define CHECK_DELEGATE(d) \
			(MUST_CHECK_REFERENCE_ONCE); \
			if (!d) { \
				static const String _methodName = __func__; \
				request.Error(_methodName + ": Invalid <" + d.GetTypeName() + "> " + #d); \
				assert(false); \
			}
#define CHECK_REFERENCES_NONE() \
			const int MUST_CHECK_REFERENCE_ONCE = 0;

#ifdef DEBUG_SCRIPT
#define CHECK_REFERENCES(d) \
			const int MUST_CHECK_REFERENCE_ONCE = 0; \
			{ \
				static const String _methodName = __func__; \
				IScript::Request::Ref refs[] = { d }; \
				for (size_t i = 0; i < sizeof(refs) / sizeof(refs[0]); i++) { \
					if (!refs[i]) { \
						char digit[32]; \
						sprintf(digit, "%d", (int)i); \
						request.Error(_methodName + ": Invalid references " #d "[" + digit + "]"); \
						request.DoLock(); \
						for (size_t j = 0; j < sizeof(refs) / sizeof(refs[0]); j++) { \
							if (refs[i]) { \
								request.Dereference(refs[i]); \
							} \
						} \
						request.UnLock(); \
						assert(false); \
					} \
				} \
			}

#define CHECK_REFERENCES_LOCKED(d) \
			const int MUST_CHECK_REFERENCE_ONCE = 0; \
			{ \
				static const String _methodName = __func__; \
				IScript::Request::Ref refs[] = { d }; \
				for (size_t i = 0; i < sizeof(refs) / sizeof(refs[0]); i++) { \
					if (!refs[i]) { \
						char digit[32]; \
						sprintf(digit, "%d", (int)i); \
						request.Error(_methodName + ": Invalid references " #d "[" + digit + "]"); \
						for (size_t j = 0; j < sizeof(refs) / sizeof(refs[0]); j++) { \
							if (refs[i]) { \
								request.Dereference(refs[i]); \
							} \
						} \
						assert(false); \
					} \
				} \
			}
#else
#define CHECK_REFERENCES(d) \
			const int MUST_CHECK_REFERENCE_ONCE = 0; 
#define CHECK_REFERENCES_LOCKED(d) \
			const int MUST_CHECK_REFERENCE_ONCE = 0; 
#endif

#ifdef DEBUG_SCRIPT
#define CHECK_REFERENCES_WITH_TYPE(d, t) \
			const int MUST_CHECK_REFERENCE_ONCE = 0; \
			{ \
				static const String _methodName = __func__; \
				IScript::Request::Ref refs[] = { d }; \
				IScript::Request::TYPE types[] = { t }; \
				static_assert(sizeof(refs) / sizeof(refs[0]) == sizeof(types) / sizeof(types[0]), "Unmatched type checking count"); \
				request.DoLock(); \
				for (size_t i = 0; i < sizeof(refs) / sizeof(refs[0]); i++) { \
					IScript::Request::TYPE p = request.GetReferenceType(refs[i]); \
					if (types[i] != IScript::Request::ANY && !request.GetScript()->IsTypeCompatible(types[i], p)) {\
						char digit[32]; \
						sprintf(digit, "%d", (int)i); \
						request.Error(_methodName + ": Invalid (or wrong type) references (" #d ") [" + digit + "], expect (" #t ")"); \
						for (size_t j = 0; j < sizeof(refs) / sizeof(refs[0]); j++) { \
							if (refs[i]) { \
								request.Dereference(refs[i]); \
							} \
						} \
						assert(false);\
					} \
				} \
				request.UnLock(); \
			}

#define CHECK_REFERENCES_WITH_TYPE_LOCKED(d, t) \
			const int MUST_CHECK_REFERENCE_ONCE = 0; \
			{ \
				static const String _methodName = __func__; \
				IScript::Request::Ref refs[] = { d }; \
				IScript::Request::TYPE types[] = { t }; \
				static_assert(sizeof(refs) / sizeof(refs[0]) == sizeof(types) / sizeof(types[0]), "Unmatched type checking count"); \
				for (size_t i = 0; i < sizeof(refs) / sizeof(refs[0]); i++) { \
					IScript::Request::TYPE p = request.GetReferenceType(refs[i]); \
					if (types[i] != IScript::Request::ANY && !request.GetScript()->IsTypeCompatible(types[i], p)) {\
						char digit[32]; \
						sprintf(digit, "%d", (int)i); \
						request.Error(_methodName + ": Invalid (or wrong type) references (" #d ") [" + digit + "], expect (" #t ")"); \
						for (size_t j = 0; j < sizeof(refs) / sizeof(refs[0]); j++) { \
							if (refs[i]) { \
								request.Dereference(refs[i]); \
							} \
						} \
						assert(false);\
					} \
				} \
			}
#else
#define CHECK_REFERENCES_WITH_TYPE(d, t) \
			const int MUST_CHECK_REFERENCE_ONCE = 0;
#define CHECK_REFERENCES_WITH_TYPE_LOCKED(d, t) \
			const int MUST_CHECK_REFERENCE_ONCE = 0;
#endif

		virtual void Reset();
		virtual const char* GetFileExt() const = 0;
		virtual IScript* NewScript() const = 0;
		virtual Request* NewRequest(const String& entry = "") = 0;
		virtual Request& GetDefaultRequest() = 0;
		virtual bool IsResetting() const = 0;
		virtual bool IsHosting() const = 0;
		virtual bool IsTypeCompatible(Request::TYPE target, Request::TYPE source) const;
		virtual void SetErrorHandler(const TWrapper<void, Request&, const String&>& errorHandler);
		virtual void SetDispatcher(const TWrapper<void, Request&, IHost*, size_t, const TWrapper<void, Request&>& >& disp);
		const TWrapper<void, Request&, IHost*, size_t, const TWrapper<void, Request&>& >& GetDispatcher() const;
		// virtual void DoLock();
		// virtual void UnLock();

		// Request pool for optimizing frequently creation and deletions.
		class RequestPool {
		public:
			RequestPool(IScript& script, uint32_t size);
			Request* allocate(size_t n);
			void construct(Request* req);
			void destroy(Request* req);
			void deallocate(Request* request, size_t n);
			IScript& GetScript();
			TPool<Request, RequestPool> requestPool;

		protected:
			IScript& script;
		};

	protected:
		TWrapper<void, Request&, IHost*, size_t, const TWrapper<void, Request&>& > dispatcher;
		TWrapper<void, Request&, const String&> errorHandler;
		friend class Request;
	};

	extern IScript::MetaLibrary ScriptLibrary;
	extern IScript::MetaMethod ScriptMethod;
	extern IScript::MetaMethod ScriptMethodLocked;
	extern IScript::MetaVariable ScriptVariable;
	extern IScript::Request::TableStart begintable;
	extern IScript::Request::TableEnd endtable;
	extern IScript::Request::ArrayStart beginarray;
	extern IScript::Request::ArrayEnd endarray;
	extern IScript::Request::Key key;
	extern IScript::Request::Nil nil;
	extern IScript::Request::Global global;
}

