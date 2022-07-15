// TProxy.h
// Small and quick function wrapper (like std::function<>)
// PaintDream (paintdream@paintdream.com)
// 2014-12-14
//
#pragma once
#include "../PaintsNow.h"

// Remarks: If you use C++ 11, all features provided in this file can be well replaced by std::function<(...)>.
// But in order to provide the same interface for both vc6 or other vs (which do not support std::function()), I wrote TProxy/TWrapper for them.

#include <cassert>
#include <cstring>
#include <set>
#include <utility>

namespace PaintsNow {
	class IHost; // Just a type monitor, no definition.
	// Any pointer to member function of a multi-derived class takes 2 * sizeof(void*) size.
	// However, when it comes to single-derived class or native class, MSVC compiler requires only half storage of it.
	// So we must create Void in forced multi-Interfaced way to prevent possible memory corruptions in TWrapper<>'s memory layout.

	template <class T>
	struct ReturnType {
		typedef T type;
	};

	template <>
	struct ReturnType<void> {
		typedef Void type;
	};

#if defined(_MSC_VER) && _MSC_VER < 1800

	struct void_type {};
	struct not_void_type {};
	template <class T>
	struct count_void_type { enum { value = 1 }; };
	template <>
	struct count_void_type<Void> { enum { value = 0 }; };

	template <class R = Void, class A = Void, class B = Void, class C = Void, class D = Void, class E = Void, class F = Void, class G = Void, class H = Void, class I = Void, class J = Void, class K = Void, class L = Void, class M = Void, class N = Void, class O = Void, class P = Void>
	class TProxy {
	public:
		static inline size_t GetCount() {
			return count_void_type<A>::value + count_void_type<B>::value + count_void_type<C>::value + count_void_type<D>::value
				+ count_void_type<E>::value + count_void_type<F>::value + count_void_type<G>::value + count_void_type<H>::value
				+ count_void_type<I>::value + count_void_type<J>::value + count_void_type<K>::value + count_void_type<L>::value
				+ count_void_type<M>::value + count_void_type<N>::value + count_void_type<O>::value + count_void_type<P>::value;
		}
	};

	template <class ManageType, class T, class R = Void, class A = Void, class B = Void, class C = Void, class D = Void, class E = Void, class F = Void, class G = Void, class H = Void, class I = Void, class J = Void, class K = Void, class L = Void, class M = Void, class N = Void, class O = Void, class P = Void>
	class TMethod : public TProxy<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> {
	public:
		typedef R(T::*ZZ)();
		typedef R(T::*ZA)(A a);
		typedef R(T::*ZB)(A a, B b);
		typedef R(T::*ZC)(A a, B b, C c);
		typedef R(T::*ZD)(A a, B b, C c, D d);
		typedef R(T::*ZE)(A a, B b, C c, D d, E e);
		typedef R(T::*ZF)(A a, B b, C c, D d, E e, F f);
		typedef R(T::*ZG)(A a, B b, C c, D d, E e, F f, G g);
		typedef R(T::*ZH)(A a, B b, C c, D d, E e, F f, G g, H h);
		typedef R(T::*ZI)(A a, B b, C c, D d, E e, F f, G g, H h, I i);
		typedef R(T::*ZJ)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j);
		typedef R(T::*ZK)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k);
		typedef R(T::*ZL)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l);
		typedef R(T::*ZM)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m);
		typedef R(T::*ZN)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n);
		typedef R(T::*ZO)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o);
		typedef R(T::*ZP)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p);

		struct Table {
			union {
				void* invoker;
				typename ReturnType<R>::type(*invokerZ)(const TMethod* t);
				typename ReturnType<R>::type(*invokerA)(const TMethod* t, A a);
				typename ReturnType<R>::type(*invokerB)(const TMethod* t, A a, B b);
				typename ReturnType<R>::type(*invokerC)(const TMethod* t, A a, B b, C c);
				typename ReturnType<R>::type(*invokerD)(const TMethod* t, A a, B b, C c, D d);
				typename ReturnType<R>::type(*invokerE)(const TMethod* t, A a, B b, C c, D d, E e);
				typename ReturnType<R>::type(*invokerF)(const TMethod* t, A a, B b, C c, D d, E e, F f);
				typename ReturnType<R>::type(*invokerG)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g);
				typename ReturnType<R>::type(*invokerH)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h);
				typename ReturnType<R>::type(*invokerI)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i);
				typename ReturnType<R>::type(*invokerJ)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j);
				typename ReturnType<R>::type(*invokerK)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k);
				typename ReturnType<R>::type(*invokerL)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l);
				typename ReturnType<R>::type(*invokerM)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m);
				typename ReturnType<R>::type(*invokerN)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n);
				typename ReturnType<R>::type(*invokerO)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o);
				typename ReturnType<R>::type(*invokerP)(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p);
			};
			void(*duplicator)(TMethod& output, const TMethod& input);
			void(*destructor)(TMethod& input);
		};

		static void CopyImpl(TMethod& output, const TMethod& input) {
			output.~TMethod();
			output.tablePointer = input.tablePointer;
			output.host = input.host;
			output.p = input.p;
		}

		static void CopyImplManaged(TMethod& output, const TMethod& input) {
			output.~TMethod();
			output.tablePointer = input.tablePointer;
			output.host = input.host == nullptr ? nullptr : reinterpret_cast<IHost*>(new T(*reinterpret_cast<T*>(input.host)));
			output.p = input.p;
		}

		static void DestroyImplManaged(TMethod& input) {
			assert(input.host != nullptr);
			delete reinterpret_cast<T*>(input.host);
		}

		static void DestroyImpl(TMethod& input) {}

		template <class T>
		static Table CreateTable(void* invoker, T) {
			Table tab;
			tab.invoker = invoker;
			tab.duplicator = &TMethod::CopyImpl;
			tab.destructor = &TMethod::DestroyImpl;
			return tab;
		}

		template <>
		static Table CreateTable(void* invoker, std::true_type) {
			Table tab;
			tab.invoker = invoker;
			tab.duplicator = &TMethod::CopyImplManaged;
			tab.destructor = &TMethod::DestroyImplManaged;
			return tab;
		}

		~TMethod() {
			if (host != nullptr) {
				tablePointer->destructor(*this);
			}
		}

		TMethod(const TMethod& rhs) {
			static Table tab = CreateTable(&TMethod::InvokerZ, ManageType());
			tablePointer = &tab;
			*this = rhs;
		}

		TMethod(rvalue<TMethod> value) {
			TMethod& rhs = value;

			tablePointer = rhs.tablePointer;
			host = rhs.host;
			p = rhs.p;

			rhs.host = nullptr; // clear destructor info.
			rhs.p = nullptr;
		}

		TMethod& operator = (const TMethod& rhs) {
			rhs.tablePointer->duplicator(*this, rhs);
			return *this;
		}

		TMethod& operator = (rvalue<TMethod> value) {
			TMethod& rhs = value;
			this->~TMethod();

			tablePointer = rhs.tablePointer;
			host = rhs.host;
			p = rhs.p;

			rhs.host = nullptr; // clear destructor info.
			rhs.p = nullptr;
			return *this;
		}

		TMethod() : host(nullptr), p(nullptr) {
			static Table tab = CreateTable(&TMethod::InvokerZ, ManageType());
			tablePointer = &tab;
		}

		TMethod(T* ptr, ZZ func) : host((IHost*)ptr), p(func) {
			static Table tab = CreateTable(&TMethod::InvokerZ, ManageType());
			tablePointer = &tab;
		}

		TMethod(T* ptr, ZA func) : host((IHost*)ptr), pa(func) {
			static Table tab = CreateTable(&TMethod::InvokerA, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZB func) : host((IHost*)ptr), pb(func) {
			static Table tab = CreateTable(&TMethod::InvokerB, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZC func) : host((IHost*)ptr), pc(func) {
			static Table tab = CreateTable(&TMethod::InvokerC, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZD func) : host((IHost*)ptr), pd(func) {
			static Table tab = CreateTable(&TMethod::InvokerD, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZE func) : host((IHost*)ptr), pe(func) {
			static Table tab = CreateTable(&TMethod::InvokerE, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZF func) : host((IHost*)ptr), pf(func) {
			static Table tab = CreateTable(&TMethod::InvokerF, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZG func) : host((IHost*)ptr), pg(func) {
			static Table tab = CreateTable(&TMethod::InvokerG, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZH func) : host((IHost*)ptr), ph(func) {
			static Table tab = CreateTable(&TMethod::InvokerH, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZI func) : host((IHost*)ptr), pi(func) {
			static Table tab = CreateTable(&TMethod::InvokerI, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZJ func) : host((IHost*)ptr), pj(func) {
			static Table tab = CreateTable(&TMethod::InvokerJ, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZK func) : host((IHost*)ptr), pk(func) {
			static Table tab = CreateTable(&TMethod::InvokerK, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZL func) : host((IHost*)ptr), pl(func) {
			static Table tab = CreateTable(&TMethod::InvokerL, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZM func) : host((IHost*)ptr), pm(func) {
			static Table tab = CreateTable(&TMethod::InvokerM, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZN func) : host((IHost*)ptr), pn(func) {
			static Table tab = CreateTable(&TMethod::InvokerN, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZO func) : host((IHost*)ptr), po(func) {
			static Table tab = CreateTable(&TMethod::InvokerO, ManageType());
			tablePointer = &tab;
		}
		TMethod(T* ptr, ZP func) : host((IHost*)ptr), pp(func) {
			static Table tab = CreateTable(&TMethod::InvokerP, ManageType());
			tablePointer = &tab;
		}

		template <class Z>
		struct Dispatch {
			IHost* host;
			union {
				ZZ p;
				ZA pa;
				ZB pb;
				ZC pc;
				ZD pd;
				ZE pe;
				ZF pf;
				ZG pg;
				ZH ph;
				ZI pi;
				ZJ pj;
				ZK pk;
				ZL pl;
				ZM pm;
				ZN pn;
				ZO po;
				ZP pp;
				void* _alignment[2];
			};

			Dispatch(IHost* h, ZZ f) : host(h), p(f) {}
			inline Z Invoke() const {
				return ((reinterpret_cast<T*>(host))->*p)();
			}

			inline Z Invoke(A a) const {
				return ((reinterpret_cast<T*>(host))->*pa)(a);
			}

			inline Z Invoke(A a, B b) const {
				return ((reinterpret_cast<T*>(host))->*pb)(a, b);
			}

			inline Z Invoke(A a, B b, C c) const {
				return ((reinterpret_cast<T*>(host))->*pc)(a, b, c);
			}

			inline Z Invoke(A a, B b, C c, D d) const {
				return ((reinterpret_cast<T*>(host))->*pd)(a, b, c, d);
			}

			inline Z Invoke(A a, B b, C c, D d, E e) const {
				return ((reinterpret_cast<T*>(host))->*pe)(a, b, c, d, e);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f) const {
				return ((reinterpret_cast<T*>(host))->*pf)(a, b, c, d, e, f);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g) const {
				return ((reinterpret_cast<T*>(host))->*pg)(a, b, c, d, e, f, g);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h) const {
				return ((reinterpret_cast<T*>(host))->*ph)(a, b, c, d, e, f, g, h);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i) const {
				return ((reinterpret_cast<T*>(host))->*pi)(a, b, c, d, e, f, g, h, i);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const {
				return ((reinterpret_cast<T*>(host))->*pj)(a, b, c, d, e, f, g, h, i, j);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const {
				return ((reinterpret_cast<T*>(host))->*pk)(a, b, c, d, e, f, g, h, i, j, k);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const {
				return ((reinterpret_cast<T*>(host))->*pl)(a, b, c, d, e, f, g, h, i, j, k, l);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const {
				return ((reinterpret_cast<T*>(host))->*pm)(a, b, c, d, e, f, g, h, i, j, k, l, m);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const {
				return ((reinterpret_cast<T*>(host))->*pn)(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const {
				return ((reinterpret_cast<T*>(host))->*po)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const {
				return ((reinterpret_cast<T*>(host))->*pp)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
			}
		};

		template <>
		struct Dispatch<void> {
			IHost* host;
			union {
				ZZ p;
				ZA pa;
				ZB pb;
				ZC pc;
				ZD pd;
				ZE pe;
				ZF pf;
				ZG pg;
				ZH ph;
				ZI pi;
				ZJ pj;
				ZK pk;
				ZL pl;
				ZM pm;
				ZN pn;
				ZO po;
				ZP pp;
				void* _alignment[2];
			};

			Dispatch<void>(IHost* h, ZZ f) : host(h), p(f) {}

			inline Void Invoke() const {
				((reinterpret_cast<T*>(host))->*p)();
				return Void();
			}

			inline Void Invoke(A a) const {
				((reinterpret_cast<T*>(host))->*pa)(a);
				return Void();
			}

			inline Void Invoke(A a, B b) const {
				((reinterpret_cast<T*>(host))->*pb)(a, b);
				return Void();
			}

			inline Void Invoke(A a, B b, C c) const {
				((reinterpret_cast<T*>(host))->*pc)(a, b, c);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d) const {
				((reinterpret_cast<T*>(host))->*pd)(a, b, c, d);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e) const {
				((reinterpret_cast<T*>(host))->*pe)(a, b, c, d, e);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f) const {
				((reinterpret_cast<T*>(host))->*pf)(a, b, c, d, e, f);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g) const {
				((reinterpret_cast<T*>(host))->*pg)(a, b, c, d, e, f, g);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h) const {
				((reinterpret_cast<T*>(host))->*ph)(a, b, c, d, e, f, g, h);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i) const {
				((reinterpret_cast<T*>(host))->*pi)(a, b, c, d, e, f, g, h, i);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const {
				((reinterpret_cast<T*>(host))->*pj)(a, b, c, d, e, f, g, h, i, j);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const {
				((reinterpret_cast<T*>(host))->*pk)(a, b, c, d, e, f, g, h, i, j, k);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const {
				((reinterpret_cast<T*>(host))->*pl)(a, b, c, d, e, f, g, h, i, j, k, l);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const {
				((reinterpret_cast<T*>(host))->*pm)(a, b, c, d, e, f, g, h, i, j, k, l, m);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const {
				((reinterpret_cast<T*>(host))->*pn)(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const {
				((reinterpret_cast<T*>(host))->*po)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const {
				((reinterpret_cast<T*>(host))->*pp)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
				return Void();
			}
		};

		static typename ReturnType<R>::type InvokerZ(const TMethod* t) {
			assert(GetCount() == 0);
			return Dispatch<R>(t->host, t->p).Invoke();
		}

		static typename ReturnType<R>::type InvokerA(const TMethod* t, A a) {
			assert(GetCount() == 1);
			return Dispatch<R>(t->host, t->p).Invoke(a);
		}

		static typename ReturnType<R>::type InvokerB(const TMethod* t, A a, B b) {
			assert(GetCount() == 2);
			return Dispatch<R>(t->host, t->p).Invoke(a, b);
		}

		static typename ReturnType<R>::type InvokerC(const TMethod* t, A a, B b, C c) {
			assert(GetCount() == 3);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c);
		}

		static typename ReturnType<R>::type InvokerD(const TMethod* t, A a, B b, C c, D d) {
			assert(GetCount() == 4);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d);
		}

		static typename ReturnType<R>::type InvokerE(const TMethod* t, A a, B b, C c, D d, E e) {
			assert(GetCount() == 5);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e);
		}

		static typename ReturnType<R>::type InvokerF(const TMethod* t, A a, B b, C c, D d, E e, F f) {
			assert(GetCount() == 6);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f);
		}

		static typename ReturnType<R>::type InvokerG(const TMethod* t, A a, B b, C c, D d, E e, F f, G g) {
			assert(GetCount() == 7);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g);
		}

		static typename ReturnType<R>::type InvokerH(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h) {
			assert(GetCount() == 8);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h);
		}

		static typename ReturnType<R>::type InvokerI(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i) {
			assert(GetCount() == 9);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i);
		}

		static typename ReturnType<R>::type InvokerJ(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) {
			assert(GetCount() == 10);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j);
		}

		static typename ReturnType<R>::type InvokerK(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) {
			assert(GetCount() == 11);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k);
		}

		static typename ReturnType<R>::type InvokerL(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) {
			assert(GetCount() == 12);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l);
		}

		static typename ReturnType<R>::type InvokerM(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) {
			assert(GetCount() == 13);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m);
		}

		static typename ReturnType<R>::type InvokerN(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) {
			assert(GetCount() == 14);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
		}

		static typename ReturnType<R>::type InvokerO(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) {
			assert(GetCount() == 15);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
		}

		static typename ReturnType<R>::type InvokerP(const TMethod* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) {
			assert(GetCount() == 16);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
		}

		typename ReturnType<R>::type operator () () const {
			return tablePointer->invokerZ(this);
		}

		typename ReturnType<R>::type operator () (A a) const {
			return tablePointer->invokerA(this, a);
		}

		typename ReturnType<R>::type operator () (A a, B b) const {
			return tablePointer->invokerB(this, a, b);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c) const {
			return tablePointer->invokerC(this, a, b, c);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d) const {
			return tablePointer->invokerD(this, a, b, c, d);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e) const {
			return tablePointer->invokerE(this, a, b, c, d, e);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f) const {
			return tablePointer->invokerF(this, a, b, c, d, e, f);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g) const {
			return tablePointer->invokerG(this, a, b, c, d, e, f, g);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h) const {
			return tablePointer->invokerH(this, a, b, c, d, e, f, g, h);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i) const {
			return tablePointer->invokerI(this, a, b, c, d, e, f, g, h, i);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const {
			return tablePointer->invokerJ(this, a, b, c, d, e, f, g, h, i, j);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const {
			return tablePointer->invokerK(this, a, b, c, d, e, f, g, h, i, j, k);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const {
			return tablePointer->invokerL(this, a, b, c, d, e, f, g, h, i, j, k, l);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const {
			return tablePointer->invokerM(this, a, b, c, d, e, f, g, h, i, j, k, l, m);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const {
			return tablePointer->invokerN(this, a, b, c, d, e, f, g, h, i, j, k, l, m, n);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const {
			return tablePointer->invokerO(this, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const {
			return tablePointer->invokerP(this, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
		}

		Table* tablePointer;
		IHost* host;
		union {
			ZZ p;
			ZA pa;
			ZB pb;
			ZC pc;
			ZD pd;
			ZE pe;
			ZF pf;
			ZG pg;
			ZH ph;
			ZI pi;
			ZJ pj;
			ZK pk;
			ZL pl;
			ZM pm;
			ZN pn;
			ZO po;
			ZP pp;
			void* _alignment[2];
		};
	};

	template <class R = Void, class A = Void, class B = Void, class C = Void, class D = Void, class E = Void, class F = Void, class G = Void, class H = Void, class I = Void, class J = Void, class K = Void, class L = Void, class M = Void, class N = Void, class O = Void, class P = Void>
	class TFunction : public TProxy<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> {
	public:
		typedef R(*ZZ)();
		typedef R(*ZA)(A a);
		typedef R(*ZB)(A a, B b);
		typedef R(*ZC)(A a, B b, C c);
		typedef R(*ZD)(A a, B b, C c, D d);
		typedef R(*ZE)(A a, B b, C c, D d, E e);
		typedef R(*ZF)(A a, B b, C c, D d, E e, F f);
		typedef R(*ZG)(A a, B b, C c, D d, E e, F f, G g);
		typedef R(*ZH)(A a, B b, C c, D d, E e, F f, G g, H h);
		typedef R(*ZI)(A a, B b, C c, D d, E e, F f, G g, H h, I i);
		typedef R(*ZJ)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j);
		typedef R(*ZK)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k);
		typedef R(*ZL)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l);
		typedef R(*ZM)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m);
		typedef R(*ZN)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n);
		typedef R(*ZO)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o);
		typedef R(*ZP)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p);

		struct Table {
			union {
				void* invoker;

				typename ReturnType<R>::type(*invokerZ)(const TFunction* t);
				typename ReturnType<R>::type(*invokerA)(const TFunction* t, A a);
				typename ReturnType<R>::type(*invokerB)(const TFunction* t, A a, B b);
				typename ReturnType<R>::type(*invokerC)(const TFunction* t, A a, B b, C c);
				typename ReturnType<R>::type(*invokerD)(const TFunction* t, A a, B b, C c, D d);
				typename ReturnType<R>::type(*invokerE)(const TFunction* t, A a, B b, C c, D d, E e);
				typename ReturnType<R>::type(*invokerF)(const TFunction* t, A a, B b, C c, D d, E e, F f);
				typename ReturnType<R>::type(*invokerG)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g);
				typename ReturnType<R>::type(*invokerH)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h);
				typename ReturnType<R>::type(*invokerI)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i);
				typename ReturnType<R>::type(*invokerJ)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j);
				typename ReturnType<R>::type(*invokerK)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k);
				typename ReturnType<R>::type(*invokerL)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l);
				typename ReturnType<R>::type(*invokerM)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m);
				typename ReturnType<R>::type(*invokerN)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n);
				typename ReturnType<R>::type(*invokerO)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o);
				typename ReturnType<R>::type(*invokerP)(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p);
			};

			void(*duplicator)(TFunction& output, const TFunction& input);
			void(*destructor)(TFunction& input);
		};

		static void CopyImpl(TFunction& output, const TFunction& input) {
			output.~TFunction();
			output.tablePointer = input.tablePointer;
			output.host = input.host;
			output.p = input.p;
		}

		static void DestroyImpl(TFunction& input) {}
		static Table CreateTable(void* invoker) {
			Table tab;
			tab.invoker = invoker;
			tab.duplicator = &TFunction::CopyImpl;
			tab.destructor = &TFunction::DestroyImpl;
			return tab;
		}

		~TFunction() {
			if (host != nullptr) {
				tablePointer->destructor(*this);
			}
		}

		TFunction(const TFunction& rhs) {
			static Table tab = CreateTable(&TFunction::InvokerZ);
			tablePointer = &tab;

			*this = rhs;
		}

		TFunction(rvalue<TFunction> value) {
			TFunction& rhs = value;

			tablePointer = rhs.tablePointer;
			host = rhs.host;
			p = rhs.p;

			rhs.host = nullptr; // clear destructor info.
			rhs.p = nullptr;
		}

		TFunction& operator = (const TFunction& rhs) {
			rhs.tablePointer->duplicator(*this, rhs);
			return *this;
		}

		TFunction& operator = (rvalue<TFunction> rhs) {
			this->~TFunction();

			TFunction& rhs = value;

			tablePointer = rhs.tablePointer;
			host = rhs.host;
			p = rhs.p;

			rhs.host = nullptr; // clear destructor info.
			rhs.p = nullptr;
			return *this;
		}

		TFunction() : host(nullptr), p(nullptr) {
			static Table tab = CreateTable(&TFunction::InvokerZ);
			tablePointer = &tab;
		}

		TFunction(ZZ func) : host(nullptr), p(func) {
			static Table tab = CreateTable(&TFunction::InvokerZ);
			tablePointer = &tab;
		}

		TFunction(ZA func) : host(nullptr), pa(func) {
			static Table tab = CreateTable(&TFunction::InvokerA);
			tablePointer = &tab;
		}

		TFunction(ZB func) : host(nullptr), pb(func) {
			static Table tab = CreateTable(&TFunction::InvokerB);
			tablePointer = &tab;
		}

		TFunction(ZC func) : host(nullptr), pc(func) {
			static Table tab = CreateTable(&TFunction::InvokerC);
			tablePointer = &tab;
		}

		TFunction(ZD func) : host(nullptr), pd(func) {
			static Table tab = CreateTable(&TFunction::InvokerD);
			tablePointer = &tab;
		}

		TFunction(ZE func) : host(nullptr), pe(func) {
			static Table tab = CreateTable(&TFunction::InvokerE);
			tablePointer = &tab;
		}

		TFunction(ZF func) : host(nullptr), pf(func) {
			static Table tab = CreateTable(&TFunction::InvokerF);
			tablePointer = &tab;
		}

		TFunction(ZG func) : host(nullptr), pg(func) {
			static Table tab = CreateTable(&TFunction::InvokerG);
			tablePointer = &tab;
		}

		TFunction(ZH func) : host(nullptr), ph(func) {
			static Table tab = CreateTable(&TFunction::InvokerH);
			tablePointer = &tab;
		}

		TFunction(ZI func) : host(nullptr), pi(func) {
			static Table tab = CreateTable(&TFunction::InvokerI);
			tablePointer = &tab;
		}

		TFunction(ZJ func) : host(nullptr), pj(func) {
			static Table tab = CreateTable(&TFunction::InvokerJ);
			tablePointer = &tab;
		}

		TFunction(ZK func) : host(nullptr), pk(func) {
			static Table tab = CreateTable(&TFunction::InvokerK);
			tablePointer = &tab;
		}

		TFunction(ZL func) : host(nullptr), pl(func) {
			static Table tab = CreateTable(&TFunction::InvokerL);
			tablePointer = &tab;
		}

		TFunction(ZM func) : host(nullptr), pm(func) {
			static Table tab = CreateTable(&TFunction::InvokerM);
			tablePointer = &tab;
		}

		TFunction(ZN func) : host(nullptr), pn(func) {
			static Table tab = CreateTable(&TFunction::InvokerN);
			tablePointer = &tab;
		}

		TFunction(ZO func) : host(nullptr), po(func) {
			static Table tab = CreateTable(&TFunction::InvokerO);
			tablePointer = &tab;
		}

		TFunction(ZP func) : host(nullptr), pp(func) {
			static Table tab = CreateTable(&TFunction::InvokerP);
			tablePointer = &tab;
		}

		template <class Z>
		struct Dispatch {
			IHost* host;
			union {
				ZZ p;
				ZA pa;
				ZB pb;
				ZC pc;
				ZD pd;
				ZE pe;
				ZF pf;
				ZG pg;
				ZH ph;
				ZI pi;
				ZJ pj;
				ZK pk;
				ZL pl;
				ZM pm;
				ZN pn;
				ZO po;
				ZP pp;
				void* _alignment[2];
			};

			Dispatch(IHost* h, ZZ f) : host(h), p(f) {}
			inline Z Invoke() const {
				return p();
			}

			inline Z Invoke(A a) const {
				return pa(a);
			}

			inline Z Invoke(A a, B b) const {
				return pb(a, b);
			}

			inline Z Invoke(A a, B b, C c) const {
				return pc(a, b, c);
			}

			inline Z Invoke(A a, B b, C c, D d) const {
				return pd(a, b, c, d);
			}

			inline Z Invoke(A a, B b, C c, D d, E e) const {
				return pe(a, b, c, d, e);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f) const {
				return pf(a, b, c, d, e, f);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g) const {
				return pg(a, b, c, d, e, f, g);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h) const {
				return ph(a, b, c, d, e, f, g, h);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i) const {
				return pi(a, b, c, d, e, f, g, h, i);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const {
				return pj(a, b, c, d, e, f, g, h, i, j);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const {
				return pk(a, b, c, d, e, f, g, h, i, j, k);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const {
				return pl(a, b, c, d, e, f, g, h, i, j, k, l);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const {
				return pm(a, b, c, d, e, f, g, h, i, j, k, l, m);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const {
				return pn(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const {
				return po(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
			}

			inline Z Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const {
				return pp(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
			}
		};

		template <>
		struct Dispatch<void> {
			IHost* host;
			union {
				ZZ p;
				ZA pa;
				ZB pb;
				ZC pc;
				ZD pd;
				ZE pe;
				ZF pf;
				ZG pg;
				ZH ph;
				ZI pi;
				ZJ pj;
				ZK pk;
				ZL pl;
				ZM pm;
				ZN pn;
				ZO po;
				ZP pp;
				void* _alignment[2];
			};

			Dispatch<void>(IHost* h, ZZ f) : host(h), p(f) {}

			inline Void Invoke() const {
				p();
				return Void();
			}

			inline Void Invoke(A a) const {
				pa(a);
				return Void();
			}

			inline Void Invoke(A a, B b) const {
				pb(a, b);
				return Void();
			}

			inline Void Invoke(A a, B b, C c) const {
				pc(a, b, c);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d) const {
				pd(a, b, c, d);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e) const {
				pe(a, b, c, d, e);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f) const {
				pf(a, b, c, d, e, f);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g) const {
				pg(a, b, c, d, e, f, g);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h) const {
				ph(a, b, c, d, e, f, g, h);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i) const {
				pi(a, b, c, d, e, f, g, h, i);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const {
				pj(a, b, c, d, e, f, g, h, i, j);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const {
				pk(a, b, c, d, e, f, g, h, i, j, k);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const {
				pl(a, b, c, d, e, f, g, h, i, j, k, l);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const {
				pm(a, b, c, d, e, f, g, h, i, j, k, l, m);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const {
				pn(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const {
				po(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
				return Void();
			}

			inline Void Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const {
				pp(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
				return Void();
			}
		};

		static typename ReturnType<R>::type InvokerZ(const TFunction* t) {
			assert(GetCount() == 0);
			return Dispatch<R>(t->host, t->p).Invoke();
		}

		static typename ReturnType<R>::type InvokerA(const TFunction* t, A a) {
			assert(GetCount() == 1);
			return Dispatch<R>(t->host, t->p).Invoke(a);
		}

		static typename ReturnType<R>::type InvokerB(const TFunction* t, A a, B b) {
			assert(GetCount() == 2);
			return Dispatch<R>(t->host, t->p).Invoke(a, b);
		}

		static typename ReturnType<R>::type InvokerC(const TFunction* t, A a, B b, C c) {
			assert(GetCount() == 3);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c);
		}

		static typename ReturnType<R>::type InvokerD(const TFunction* t, A a, B b, C c, D d) {
			assert(GetCount() == 4);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d);
		}

		static typename ReturnType<R>::type InvokerE(const TFunction* t, A a, B b, C c, D d, E e) {
			assert(GetCount() == 5);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e);
		}

		static typename ReturnType<R>::type InvokerF(const TFunction* t, A a, B b, C c, D d, E e, F f) {
			assert(GetCount() == 6);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f);
		}

		static typename ReturnType<R>::type InvokerG(const TFunction* t, A a, B b, C c, D d, E e, F f, G g) {
			assert(GetCount() == 7);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g);
		}

		static typename ReturnType<R>::type InvokerH(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h) {
			assert(GetCount() == 8);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h);
		}

		static typename ReturnType<R>::type InvokerI(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i) {
			assert(GetCount() == 9);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i);
		}

		static typename ReturnType<R>::type InvokerJ(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) {
			assert(GetCount() == 10);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j);
		}

		static typename ReturnType<R>::type InvokerK(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) {
			assert(GetCount() == 11);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k);
		}

		static typename ReturnType<R>::type InvokerL(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) {
			assert(GetCount() == 12);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l);
		}

		static typename ReturnType<R>::type InvokerM(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) {
			assert(GetCount() == 13);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m);
		}

		static typename ReturnType<R>::type InvokerN(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) {
			assert(GetCount() == 14);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
		}

		static typename ReturnType<R>::type InvokerO(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) {
			assert(GetCount() == 15);
			return Dispatch<R>(t->host, t->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
		}

		static typename ReturnType<R>::type InvokerP(const TFunction* t, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) {
			assert(GetCount() == 16);
			return Dispatch<R>(host, this->p).Invoke(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
		}

		typename ReturnType<R>::type operator () () const {
			return tablePointer->invokerZ(this);
		}

		typename ReturnType<R>::type operator () (A a) const {
			return tablePointer->invokerA(this, a);
		}

		typename ReturnType<R>::type operator () (A a, B b) const {
			return tablePointer->invokerB(this, a, b);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c) const {
			return tablePointer->invokerC(this, a, b, c);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d) const {
			return tablePointer->invokerD(this, a, b, c, d);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e) const {
			return tablePointer->invokerE(this, a, b, c, d, e);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f) const {
			return tablePointer->invokerF(this, a, b, c, d, e, f);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g) const {
			return tablePointer->invokerG(this, a, b, c, d, e, f, g);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h) const {
			return tablePointer->invokerH(this, a, b, c, d, e, f, g, h);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i) const {
			return tablePointer->invokerI(this, a, b, c, d, e, f, g, h, i);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const {
			return tablePointer->invokerJ(this, a, b, c, d, e, f, g, h, i, j);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const {
			return tablePointer->invokerK(this, a, b, c, d, e, f, g, h, i, j, k);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const {
			return tablePointer->invokerL(this, a, b, c, d, e, f, g, h, i, j, k, l);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const {
			return tablePointer->invokerM(this, a, b, c, d, e, f, g, h, i, j, k, l, m);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const {
			return tablePointer->invokerN(this, a, b, c, d, e, f, g, h, i, j, k, l, m, n);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const {
			return tablePointer->invokerO(this, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
		}

		typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const {
			return tablePointer->invokerP(this, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
		}

		Table* tablePointer;
		IHost* host;
		union {
			ZZ p;
			ZA pa;
			ZB pb;
			ZC pc;
			ZD pd;
			ZE pe;
			ZF pf;
			ZG pg;
			ZH ph;
			ZI pi;
			ZJ pj;
			ZK pk;
			ZL pl;
			ZM pm;
			ZN pn;
			ZO po;
			ZP pp;
			void* _alignment[2];
		};
	};

	// TWrapper is the same as TMethod, without template class type T.
	template <class R = Void, class A = Void, class B = Void, class C = Void, class D = Void, class E = Void, class F = Void, class G = Void, class H = Void, class I = Void, class J = Void, class K = Void, class L = Void, class M = Void, class N = Void, class O = Void, class P = Void>
	class TWrapper {
	public:
		typedef R _R; typedef A _A; typedef B _B; typedef C _C;
		typedef D _D; typedef E _E; typedef F _F; typedef G _G;
		typedef H _H; typedef I _I; typedef J _J; typedef K _K;
		typedef L _L; typedef M _M; typedef N _N; typedef O _O;
		typedef P _P;
	private:
		template <class T>
		void Init(T t) {
			TFunction<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> func(t);
			TMethod<std::false_type, Void, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& m = reinterpret_cast<TMethod<std::false_type, Void, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>&>(func);
			proxy = m;
		}
	public:
		TWrapper() {}
		TWrapper(const int) {} // for nullptr

		TWrapper(R(*d)()) {
			Init(d);
		}

		TWrapper(R(*d)(A)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I, J)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I, J, K)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I, J, K, L)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I, J, K, L, M)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I, J, K, L, M, N)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O)) {
			Init(d);
		}

		TWrapper(R(*d)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P)) {
			Init(d);
		}

		TWrapper(const TProxy<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& t) {
			const TMethod<std::false_type, Void, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& m = reinterpret_cast<const TMethod<std::false_type, Void, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>&>(t);
			proxy = m;
		}

		template <class Z>
		TWrapper(const TWrapper<Z, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& other) {
			*this = reinterpret_cast<const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>&>(other);
		}

		operator bool() const {
			return proxy.p != nullptr;
		}

		void Clear() {
			proxy.p = nullptr;
		}

		inline IHost* GetHost() const { return proxy.host; }
		inline size_t GetCount() const { return proxy.GetCount(); }

		bool operator < (const TWrapper& rhs) const {
			if (proxy.host < rhs.proxy.host) {
				return true;
			} else if (proxy.host > rhs.proxy.host) {
				return false;
			} else {
				return *(void**)&proxy.p < *(void**)&rhs.proxy.p;
			}
		}

		const TProxy<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& GetProxy() const {
			return proxy;
		}

		inline typename ReturnType<R>::type operator () () const {
			return proxy();
		}

		inline typename ReturnType<R>::type operator () (A a) const {
			return proxy(a);
		}

		inline typename ReturnType<R>::type operator () (A a, B b) const {
			return proxy(a, b);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c) const {
			return proxy(a, b, c);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d) const {
			return proxy(a, b, c, d);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e) const {
			return proxy(a, b, c, d, e);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f) const {
			return proxy(a, b, c, d, e, f);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g) const {
			return proxy(a, b, c, d, e, f, g);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h) const {
			return proxy(a, b, c, d, e, f, g, h);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i) const {
			return proxy(a, b, c, d, e, f, g, h, i);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const {
			return proxy(a, b, c, d, e, f, g, h, i, j);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const {
			return proxy(a, b, c, d, e, f, g, h, i, j, k);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const {
			return proxy(a, b, c, d, e, f, g, h, i, j, k, l);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const {
			return proxy(a, b, c, d, e, f, g, h, i, j, k, l, m);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const {
			return proxy(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const {
			return proxy(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
		}

		inline typename ReturnType<R>::type operator () (A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const {
			return proxy(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
		}

		TMethod<std::false_type, Void, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> proxy;
	};

	template <class T, class R>
	TWrapper<R> Wrap(T* t, R(T::*d)()) {
		return TWrapper<R>(TMethod<std::false_type, T, R>(t, d));
	}

	template <class T, class R, class A>
	TWrapper<R, A> Wrap(T* t, R(T::*d)(A a)) {
		return TWrapper<R, A>(TMethod<std::false_type, T, R, A>(t, d));
	}

	template <class T, class R, class A, class B>
	TWrapper<R, A, B> Wrap(T* t, R(T::*d)(A a, B b)) {
		return TWrapper<R, A, B>(TMethod<std::false_type, T, R, A, B>(t, d));
	}

	template <class T, class R, class A, class B, class C>
	TWrapper<R, A, B, C> Wrap(T* t, R(T::*d)(A a, B b, C c)) {
		return TWrapper<R, A, B, C>(TMethod<std::false_type, T, R, A, B, C>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D>
	TWrapper<R, A, B, C, D> Wrap(T* t, R(T::*d)(A a, B b, C c, D d)) {
		return TWrapper<R, A, B, C, D>(TMethod<std::false_type, T, R, A, B, C, D>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E>
	TWrapper<R, A, B, C, D, E> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e)) {
		return TWrapper<R, A, B, C, D, E>(TMethod<std::false_type, T, R, A, B, C, D, E>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F>
	TWrapper<R, A, B, C, D, E, F> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f)) {
		return TWrapper<R, A, B, C, D, E, F>(TMethod<std::false_type, T, R, A, B, C, D, E, F>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G>
	TWrapper<R, A, B, C, D, E, F, G> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g)) {
		return TWrapper<R, A, B, C, D, E, F, G>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H>
	TWrapper<R, A, B, C, D, E, F, G, H> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h)) {
		return TWrapper<R, A, B, C, D, E, F, G, H>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I>
	TWrapper<R, A, B, C, D, E, F, G, H, I> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(t, d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(t, d));
	}

	template <class T, class R>
	TWrapper<R> Wrap(T* t, R(T::*d)() const) {
		typedef R(T::*org)();
		return TWrapper<R>(TMethod<std::false_type, T, R>(t, (org)d));
	}

	template <class T, class R, class A>
	TWrapper<R, A> Wrap(T* t, R(T::*d)(A a) const) {
		typedef R(T::*org)(A);
		return TWrapper<R, A>(TMethod<std::false_type, T, R, A>(t, (org)d));
	}

	template <class T, class R, class A, class B>
	TWrapper<R, A, B> Wrap(T* t, R(T::*d)(A a, B b) const) {
		typedef R(T::*org)(A, B);
		return TWrapper<R, A, B>(TMethod<std::false_type, T, R, A, B>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C>
	TWrapper<R, A, B, C> Wrap(T* t, R(T::*d)(A a, B b, C c) const) {
		typedef R(T::*org)(A, B, C);
		return TWrapper<R, A, B, C>(TMethod<std::false_type, T, R, A, B, C>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D>
	TWrapper<R, A, B, C, D> Wrap(T* t, R(T::*d)(A a, B b, C c, D d) const) {
		typedef R(T::*org)(A, B, C, D);
		return TWrapper<R, A, B, C, D>(TMethod<std::false_type, T, R, A, B, C, D>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E>
	TWrapper<R, A, B, C, D, E> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e) const) {
		typedef R(T::*org)(A, B, C, D, E);
		return TWrapper<R, A, B, C, D, E>(TMethod<std::false_type, T, R, A, B, C, D, E>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F>
	TWrapper<R, A, B, C, D, E, F> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f) const) {
		typedef R(T::*org)(A, B, C, D, E, F);
		return TWrapper<R, A, B, C, D, E, F>(TMethod<std::false_type, T, R, A, B, C, D, E, F>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G>
	TWrapper<R, A, B, C, D, E, F, G> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G);
		return TWrapper<R, A, B, C, D, E, F, G>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H>
	TWrapper<R, A, B, C, D, E, F, G, H> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H);
		return TWrapper<R, A, B, C, D, E, F, G, H>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I>
	TWrapper<R, A, B, C, D, E, F, G, H, I> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I);
		return TWrapper<R, A, B, C, D, E, F, G, H, I>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M, N);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(t, (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> Wrap(T* t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(TMethod<std::false_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(t, (org)d));
	}

	// closure
	template <class T, class R>
	TWrapper<R> WrapClosure(rvalue<T> t, R(T::*d)()) {
		return TWrapper<R>(TMethod<std::true_type, T, R>(new T(t), d));
	}

	template <class T, class R, class A>
	TWrapper<R, A> WrapClosure(rvalue<T> t, R(T::*d)(A a)) {
		return TWrapper<R, A>(TMethod<std::true_type, T, R, A>(new T(t), d));
	}

	template <class T, class R, class A, class B>
	TWrapper<R, A, B> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b)) {
		return TWrapper<R, A, B>(TMethod<std::true_type, T, R, A, B>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C>
	TWrapper<R, A, B, C> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c)) {
		return TWrapper<R, A, B, C>(TMethod<std::true_type, T, R, A, B, C>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D>
	TWrapper<R, A, B, C, D> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d)) {
		return TWrapper<R, A, B, C, D>(TMethod<std::true_type, T, R, A, B, C, D>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E>
	TWrapper<R, A, B, C, D, E> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e)) {
		return TWrapper<R, A, B, C, D, E>(TMethod<std::true_type, T, R, A, B, C, D, E>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F>
	TWrapper<R, A, B, C, D, E, F> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f)) {
		return TWrapper<R, A, B, C, D, E, F>(TMethod<std::true_type, T, R, A, B, C, D, E, F>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G>
	TWrapper<R, A, B, C, D, E, F, G> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g)) {
		return TWrapper<R, A, B, C, D, E, F, G>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H>
	TWrapper<R, A, B, C, D, E, F, G, H> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h)) {
		return TWrapper<R, A, B, C, D, E, F, G, H>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I>
	TWrapper<R, A, B, C, D, E, F, G, H, I> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(new T(t), d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(new T(t), d));
	}

	template <class T, class R>
	TWrapper<R> WrapClosure(rvalue<T> t, R(T::*d)() const) {
		typedef R(T::*org)();
		return TWrapper<R>(TMethod<std::true_type, T, R>(new T(t), (org)d));
	}

	template <class T, class R, class A>
	TWrapper<R, A> WrapClosure(rvalue<T> t, R(T::*d)(A a) const) {
		typedef R(T::*org)(A);
		return TWrapper<R, A>(TMethod<std::true_type, T, R, A>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B>
	TWrapper<R, A, B> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b) const) {
		typedef R(T::*org)(A, B);
		return TWrapper<R, A, B>(TMethod<std::true_type, T, R, A, B>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C>
	TWrapper<R, A, B, C> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c) const) {
		typedef R(T::*org)(A, B, C);
		return TWrapper<R, A, B, C>(TMethod<std::true_type, T, R, A, B, C>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D>
	TWrapper<R, A, B, C, D> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d) const) {
		typedef R(T::*org)(A, B, C, D);
		return TWrapper<R, A, B, C, D>(TMethod<std::true_type, T, R, A, B, C, D>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E>
	TWrapper<R, A, B, C, D, E> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e) const) {
		typedef R(T::*org)(A, B, C, D, E);
		return TWrapper<R, A, B, C, D, E>(TMethod<std::true_type, T, R, A, B, C, D, E>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F>
	TWrapper<R, A, B, C, D, E, F> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f) const) {
		typedef R(T::*org)(A, B, C, D, E, F);
		return TWrapper<R, A, B, C, D, E, F>(TMethod<std::true_type, T, R, A, B, C, D, E, F>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G>
	TWrapper<R, A, B, C, D, E, F, G> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G);
		return TWrapper<R, A, B, C, D, E, F, G>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H>
	TWrapper<R, A, B, C, D, E, F, G, H> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H);
		return TWrapper<R, A, B, C, D, E, F, G, H>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I>
	TWrapper<R, A, B, C, D, E, F, G, H, I> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I);
		return TWrapper<R, A, B, C, D, E, F, G, H, I>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M, N);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(new T(t), (org)d));
	}

	template <class T, class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> WrapClosure(rvalue<T> t, R(T::*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) const) {
		typedef R(T::*org)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P);
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(TMethod<std::true_type, T, R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(new T(t), (org)d));
	}

	// function

	template <class R>
	TWrapper<R> Wrap(R(*d)()) {
		return TWrapper<R>(d);
	}

	template <class R, class A>
	TWrapper<R, A> Wrap(R(*d)(A a)) {
		return TWrapper<R, A>(d);
	}

	template <class R, class A, class B>
	TWrapper<R, A, B> Wrap(R(*d)(A a, B b)) {
		return TWrapper<R, A, B>(d);
	}

	template <class R, class A, class B, class C>
	TWrapper<R, A, B, C> Wrap(R(*d)(A a, B b, C c)) {
		return TWrapper<R, A, B, C>(d);
	}

	template <class R, class A, class B, class C, class D>
	TWrapper<R, A, B, C, D> Wrap(R(*d)(A a, B b, C c, D d)) {
		return TWrapper<R, A, B, C, D>(d);
	}

	template <class R, class A, class B, class C, class D, class E>
	TWrapper<R, A, B, C, D, E> Wrap(R(*d)(A a, B b, C c, D d, E e)) {
		return TWrapper<R, A, B, C, D, E>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F>
	TWrapper<R, A, B, C, D, E, F> Wrap(R(*d)(A a, B b, C c, D d, E e, F f)) {
		return TWrapper<R, A, B, C, D, E, F>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G>
	TWrapper<R, A, B, C, D, E, F, G> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g)) {
		return TWrapper<R, A, B, C, D, E, F, G>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H>
	TWrapper<R, A, B, C, D, E, F, G, H> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h)) {
		return TWrapper<R, A, B, C, D, E, F, G, H>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I>
	TWrapper<R, A, B, C, D, E, F, G, H, I> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(d);
	}

	template <class R, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
	TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> Wrap(R(*d)(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p)) {
		return TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(d);
	}
#else
	// Base class of proxies
	template <typename R = Void, typename... Args>
	class TProxy {
	public:
		static inline size_t GetCount() { return sizeof...(Args); }
	};

	// Dispatch calls to generic wrapper, with non-void returning value
	template <class T, class M, class Z, class... Args>
	struct Dispatch {
		Dispatch(const M& t) : p(t) {}

		template <typename... Params>
		inline Z Invoke(Params&&... params) const {
			return ((reinterpret_cast<T*>(p.host))->*(p.p))(std::forward<Params>(params)...);
		}
		const M& p;
	};

	// Also with void returning value
	template <class T, class M, class... Args>
	struct Dispatch<T, M, void, Args...> {
		Dispatch(const M& t) : p(t) {}

		template <typename... Params>
		inline Void Invoke(Params&&... params) {
			((reinterpret_cast<T*>(p.host))->*(p.p))(std::forward<Params>(params)...);
			return Void();
		}
		const M& p;
	};

	// Wraps member functions (methods), strictly memory overridden of TWrapper<>
	template <bool manageHost, typename T, typename R = Void, typename... Args>
	class TMethod : public TProxy<R, Args...> {
	public:
		typedef R(T::*FUNC)(Args...);
		struct Table {
			// call function
			typename ReturnType<R>::type(*invoker)(const TMethod*, Args&&... args);
			// duplicate TWrapper
			void(*duplicator)(TMethod& output, const TMethod& input);
			void(*destructor)(TMethod& input);
		};

		void InitTablePointer() {
			static Table tabManage = { &TMethod::Invoke, &TMethod::Copy, &TMethod::Destroy };
			tablePointer = &tabManage;
		}

		bool operator == (const TMethod& rhs) const {
			return host == rhs.host && p == rhs.p;
		}

		TMethod(T* ptr = nullptr, FUNC func = nullptr) : tablePointer(nullptr), host((IHost*)ptr), p(func) {
			InitTablePointer();
		}

		TMethod(const TMethod& rhs) {
			InitTablePointer();
			*this = rhs;
		}

		TMethod(TMethod&& rhs) {
			tablePointer = rhs.tablePointer;
			host = rhs.host;
			p = rhs.p;

			rhs.host = nullptr; // clear destructor info.	
			rhs.p = nullptr;
		}

		~TMethod() {
			if (host != nullptr) {
				tablePointer->destructor(*this);
			}
		}

		// Copy with none-hosted
		template <bool host>
		static typename std::enable_if<!host>::type CopyImpl(TMethod& output, const TMethod& input) {
			output.~TMethod();
			output.tablePointer = input.tablePointer;
			output.host = input.host;
			output.p = input.p;
		}

		// Copy with hosted
		template <bool host>
		static typename std::enable_if<host>::type CopyImpl(TMethod& output, const TMethod& input) {
			output.~TMethod();
			output.tablePointer = input.tablePointer;
			output.host = reinterpret_cast<IHost*>(new T(*reinterpret_cast<T*>(input.host)));
			output.p = input.p;
		}

		// Generic copy
		static void Copy(TMethod& output, const TMethod& input) {
			CopyImpl<manageHost>(output, input);
		}

		// Destroy with non-hosted
		template <bool host>
		static typename std::enable_if<!host>::type DestroyImpl(TMethod& input) {}

		// Destroy with hosted
		template <bool host>
		static typename std::enable_if<host>::type DestroyImpl(TMethod& input) {
			assert(input.host != nullptr);
			// holding the host instance?
			delete reinterpret_cast<T*>(input.host);
		}

		static void Destroy(TMethod& input) {
			DestroyImpl<manageHost>(input);
		}

		// Dispatch call, regardless of return type (void or non-void)
		static typename ReturnType<R>::type Invoke(const TMethod* m, Args&&... args) {
			return Dispatch<T, TMethod<manageHost, T, R, Args...>, R, Args...>(*m).Invoke(std::forward<Args>(args)...);
		}

		// Dynamic call with invoker
		typename ReturnType<R>::type operator () (Args&&... args) const {
			return tablePointer->invoker(this, std::forward<Args>(args)...);
		}

		TMethod& operator = (const TMethod& rhs) {
			rhs.tablePointer->duplicator(*this, rhs);
			return *this;
		}

		TMethod& operator = (TMethod&& rhs) {
			this->~TMethod();

			tablePointer = rhs.tablePointer;
			host = rhs.host;
			p = rhs.p;

			rhs.host = nullptr; // clear destructor info.
			rhs.p = nullptr;
			return *this;
		}

		Table* tablePointer;
		IHost* host;
		union {
			FUNC p;
			void* _alignment[2];
		};
	};

	// Wraps C-functions, strictly memory overridden of TWrapper<>
	template <typename R = Void, typename... Args>
	class TFunction : public TProxy<R, Args...> {
	public:
		typedef R(*FUNC)(Args...);
		struct Table {
			typename ReturnType<R>::type(*invoker)(const TFunction*, Args&&... args);
			void(*duplicator)(TFunction& output, const TFunction& input);
			void(*destructor)(TFunction& input);
		};

		static void Copy(TFunction& output, const TFunction& input) {
			output.tablePointer = input.tablePointer;
			output.host = input.host;
			output.p = input.p;
		}

		static void Destroy(TFunction& input) {}

		bool operator == (const TFunction& rhs) const {
			return p == rhs.p;
		}

		TFunction(FUNC func = nullptr) : tablePointer(nullptr), host(nullptr), p(func) {
			Assign(ReturnType<R>());
		}

		template <class T>
		void Assign(ReturnType<T>) {
			static Table tab = { Invoke, Copy, Destroy };
			tablePointer = &tab;
		}

#ifdef _MSC_VER
		template <>
#endif
		void Assign(ReturnType<void>) {
			static Table tab = { InvokeNoReturn, Copy, Destroy };
			tablePointer = &tab;
		}

		static typename ReturnType<R>::type Invoke(const TFunction* m, Args&&... args) {
			return m->p(std::forward<Args>(args)...);
		}

		static typename ReturnType<void>::type InvokeNoReturn(const TFunction* m, Args&&... args) {
			m->p(std::forward<Args>(args)...);
			return Void();
		}

		Table* tablePointer;
		IHost* host;
		union {
			FUNC p;
			void* _alignment[2];
		};
	};

	// Generic Wrapper structure, accepts both TMethod and TFunction
	template <typename R, typename... Args>
	class TWrapper {
	public:
		TWrapper() {}
		TWrapper(std::nullptr_t) {}

		TWrapper(const TWrapper& rhs) {
			proxy = rhs.proxy;
		}

		TWrapper(TWrapper&& rhs) {
			proxy = std::move(rhs.proxy);
		}

		template <bool v, class T>
		TWrapper(TMethod<v, T, R, Args...>&& t) {
			static_assert(sizeof(t) == sizeof(proxy), "Must be the same size.");
			proxy = std::move(reinterpret_cast<TMethod<false, Void, R, Args...>&>(t));
		}

		TWrapper(TFunction<R, Args...>&& t) {
			static_assert(sizeof(t) == sizeof(proxy), "Must be the same size.");
			proxy = std::move(reinterpret_cast<TMethod<false, Void, R, Args...>&>(t));
		}

		TWrapper(R(*d)(Args...)) : TWrapper(TFunction<R, Args...>(d)) {}

		template <class G>
		operator TWrapper<G, Args...>&() {
			static_assert(std::is_convertible<R, G>::value, "Invalid conversion");
			return reinterpret_cast<TWrapper<G, Args...>&>(*this);
		}

		template <class G>
		operator const TWrapper<G, Args...>&() const {
			static_assert(std::is_convertible<R, G>::value, "Invalid conversion");
			return reinterpret_cast<const TWrapper<G, Args...>&>(*this);
		}

		TWrapper& operator = (TWrapper&& rhs) {
			proxy = std::move(rhs.proxy);
			return *this;
		}

		TWrapper& operator = (const TWrapper& rhs) {
			proxy = rhs.proxy;
			return *this;
		}

		bool operator == (const TWrapper& rhs) const {
			return proxy == rhs.proxy;
		}

		void Clear() {
			proxy.~TMethod<false, Void, R, Args...>();
			proxy.host = nullptr;
		}

		operator bool() const {
			return proxy.p != nullptr;
		}

		bool operator < (const TWrapper& rhs) const {
			if (proxy.host < rhs.proxy.host) {
				return true;
			} else if (proxy.host > rhs.proxy.host) {
				return false;
			} else {
				return *(void**)&proxy.p < *(void**)&rhs.proxy.p;
			}
		}

		inline typename ReturnType<R>::type operator () (Args... args) const {
			return proxy(std::forward<Args>(args)...);
		}

		inline IHost* GetHost() const { return proxy.host; }
		const TProxy<R, Args...>& GetProxy() const { return proxy; }
		inline size_t GetCount() const { return proxy.GetCount(); }

		TMethod<false, Void, R, Args...> proxy;
	};

	template <typename T, typename R, typename... Args>
	TWrapper<R, Args...> Wrap(const T* t, R(T::*d)(Args...)) {
		return TWrapper<R, Args...>(TMethod<false, T, R, Args...>(const_cast<T*>(t), d));
	}

	template <typename T, typename R, typename... Args>
	TWrapper<R, Args...> Wrap(const T* t, R(T::*d)(Args...) const) {
		typedef R(T::*org)(Args...);
		return TWrapper<R, Args...>(TMethod<false, T, R, Args...>(const_cast<T*>(t), (org)d));
	}

	template <typename R, typename... Args>
	TWrapper<R, Args...> Wrap(R(*d)(Args...)) {
		return TWrapper<R, Args...>(d);
	}

	template <typename T, typename R, typename... Args>
	TWrapper<R, Args...> WrapClosure(T&& t, R(std::decay<T>::type::*d)(Args...)) {
		typedef typename std::decay<T>::type TT;
		typedef R(TT::*org)(Args...);
		return TWrapper<R, Args...>(TMethod<true, TT, R, Args...>(new TT(std::forward<T>(t)), (org)d));
	}

	template <typename T, typename R, typename... Args>
	TWrapper<R, Args...> WrapClosure(T&& t, R(std::decay<T>::type::*d)(Args...) const) {
		typedef typename std::decay<T>::type TT;
		typedef R(TT::*org)(Args...);
		return TWrapper<R, Args...>(TMethod<true, TT, R, Args...>(new TT(std::forward<T>(t)), (org)d));
	}

	// Wraps lambdas. Be aware of object's life-scope !!!
	template <typename T>
	auto WrapClosure(T&& object) -> decltype(WrapClosure(std::forward<T>(object), &T::operator())) {
		return WrapClosure(std::forward<T>(object), &T::operator());
	}
#endif

#if defined(_MSC_VER) && _MSC_VER < 1800
	template <class R = Void, class A = Void, class B = Void, class C = Void, class D = Void, class E = Void, class F = Void, class G = Void, class H = Void, class I = Void, class J = Void, class K = Void, class L = Void, class M = Void, class N = Void, class O = Void, class P = Void>
	class TBinder {
	public:
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun) : func(fun) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa) : func(fun), a(aa) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb) : func(fun), a(aa), b(bb) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc) : func(fun), a(aa), b(bb), c(cc) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd) : func(fun), a(aa), b(bb), c(cc), d(dd) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh) :func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii, J jj) :func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii), j(jj) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii, J jj, K kk) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii), j(jj), k(kk) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii, J jj, K kk, L ll) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii), j(jj), k(kk), l(ll) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii, J jj, K kk, L ll, M mm) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii), j(jj), k(kk), l(ll), m(mm) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii, J jj, K kk, L ll, M mm, N nn) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii), j(jj), k(kk), l(ll), m(mm), n(nn) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii, J jj, K kk, L ll, M mm, N nn, O oo) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii), j(jj), k(kk), l(ll), m(mm), n(nn), O(oo) {}
		TBinder(const TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>& fun, A aa, B bb, C cc, D dd, E ee, F ff, G gg, H hh, I ii, J jj, K kk, L ll, M mm, N nn, O oo, P pp) : func(fun), a(aa), b(bb), c(cc), d(dd), e(ee), f(ff), g(gg), h(hh), i(ii), j(jj), k(kk), l(ll), m(mm), n(nn), O(oo), P(pp) {}

		typename ReturnType<R>::type operator () () {
			switch (func.GetCount()) {
			case 0:
				return func();
			case 1:
				return func(a);
			case 2:
				return func(a, b);
			case 3:
				return func(a, b, c);
			case 4:
				return func(a, b, c, d);
			case 5:
				return func(a, b, c, d, e);
			case 6:
				return func(a, b, c, d, e, f);
			case 7:
				return func(a, b, c, d, e, f, g);
			case 8:
				return func(a, b, c, d, e, f, g, h);
			case 9:
				return func(a, b, c, d, e, f, g, h, i);
			case 10:
				return func(a, b, c, d, e, f, g, h, i, j);
			case 11:
				return func(a, b, c, d, e, f, g, h, i, j, k);
			case 12:
				return func(a, b, c, d, e, f, g, h, i, j, k, l);
			case 13:
				return func(a, b, c, d, e, f, g, h, i, j, k, l, m);
			case 14:
				return func(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
			case 15:
				return func(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
			case 16:
				return func(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
			default:
				assert(false);
				return ReturnType<R>::type();
			}
		}

	protected:
		TWrapper<R, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> func;
		std::decay<A>::type a; std::decay<B>::type b; std::decay<C>::type c; std::decay<D>::type d; std::decay<E>::type e;
		std::decay<F>::type f; std::decay<G>::type g; std::decay<H>::type h; std::decay<I>::type i; std::decay<J>::type j;
		std::decay<K>::type k; std::decay<L>::type l; std::decay<M>::type m; std::decay<N>::type n; std::decay<O>::type o;
		std::decay<P>::type p;
	};

	template <class T>
	struct _New0 {
		static T* Invoke() {
			return new T();
		}
	};

	template <class T, class A>
	struct _New1 {
		static T* Invoke(A a) {
			return new T(a);
		}
	};

	template <class T, class A, class B>
	struct _New2 {
		static T* Invoke(A a, B b) {
			return new T(a, b);
		}
	};

	template <class T, class A, class B, class C>
	struct _New3 {
		static T* Invoke(A a, B b, C c) {
			return new T(a, b, c);
		}
	};

	template <class T, class A, class B, class C, class D>
	struct _New4 {
		static T* Invoke(A a, B b, C c, D d) {
			return new T(a, b, c, d);
		}
	};

	template <class T, class A, class B, class C, class D, class E>
	struct _New5 {
		static T* Invoke(A a, B b, C c, D d, E e) {
			return new T(a, b, c, d, e);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F>
	struct _New6 {
		static T* Invoke(A a, B b, C c, D d, E e, F f) {
			return new T(a, b, c, d, e, f);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	struct _New7 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g) {
			return new T(a, b, c, d, e, f, g);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H>
	struct _New8 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h) {
			return new T(a, b, c, d, e, f, g, h);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
	struct _New9 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i) {
			return new T(a, b, c, d, e, f, g, h, i);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
	struct _New10 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) {
			return new T(a, b, c, d, e, f, g, h, i, j);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
	struct _New11 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) {
			return new T(a, b, c, d, e, f, g, h, i, j, k);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
	struct _New12 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) {
			return new T(a, b, c, d, e, f, g, h, i, j, k, l);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
	struct _New13 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) {
			return new T(a, b, c, d, e, f, g, h, i, j, k, l, m);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
	struct _New14 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) {
			return new T(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
	struct _New15 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) {
			return new T(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
		}
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
	struct _New16 {
		static T* Invoke(A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) {
			return new T(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
		}
	};

	template <class T>
	TWrapper<T*> WrapFactory(TypeTrait<T>) {
		return Wrap(&_New0<T>::Invoke);
	}

	template <class T, class A>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a) {
		TWrapper<T*, A> wrapper = Wrap(&_New1<T, A>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A>, T*>(new TBinder<T*, A>(wrapper, a), &TBinder<T*, A>::operator ()));
	}

	template <class T, class A, class B>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b) {
		TWrapper<T*, A, B> wrapper = Wrap(&_New2<T, A, B>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B>, T*>(new TBinder<T*, A, B>(wrapper, a, b), &TBinder<T*, A, B>::operator ()));
	}

	template <class T, class A, class B, class C>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c) {
		TWrapper<T*, A, B, C> wrapper = Wrap(&_New3<T, A, B, C>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C>, T*>(new TBinder<T*, A, B, C>(wrapper, a, b, c), &TBinder<T*, A, B, C>::operator ()));
	}

	template <class T, class A, class B, class C, class D>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d) {
		TWrapper<T*, A, B, C, D> wrapper = Wrap(&_New4<T, A, B, C, D>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D>, T*>(new TBinder<T*, A, B, C, D>(wrapper, a, b, c, d), &TBinder<T*, A, B, C, D>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e) {
		TWrapper<T*, A, B, C, D, E> wrapper = Wrap(&_New5<T, A, B, C, D, E>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E>, T*>(new TBinder<T*, A, B, C, D, E>(wrapper, a, b, c, d, e), &TBinder<T*, A, B, C, D, E>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f) {
		TWrapper<T*, A, B, C, D, E, F> wrapper = Wrap(&_New6<T, A, B, C, D, E, F>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F>, T*>(new TBinder<T*, A, B, C, D, E, F>(wrapper, a, b, c, d, e, f), &TBinder<T*, A, B, C, D, E, F>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g) {
		TWrapper<T*, A, B, C, D, E, F, G> wrapper = Wrap(&_New7<T, A, B, C, D, E, F, G>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G>, T*>(new TBinder<T*, A, B, C, D, E, F, G>(wrapper, a, b, c, d, e, f, g), &TBinder<T*, A, B, C, D, E, F, G>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h) {
		TWrapper<T*, A, B, C, D, E, F, G, H> wrapper = Wrap(&_New8<T, A, B, C, D, E, F, G, H>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H>(wrapper, a, b, c, d, e, f, g, h), &TBinder<T*, A, B, C, D, E, F, G, H>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I> wrapper = Wrap(&_New9<T, A, B, C, D, E, F, G, H, I>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I>(wrapper, a, b, c, d, e, f, g, h, i), &TBinder<T*, A, B, C, D, E, F, G, H, I>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I, J> wrapper = Wrap(&_New10<T, A, B, C, D, E, F, G, H, I, J>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I, J>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I, J>(wrapper, a, b, c, d, e, f, g, h, i, j), &TBinder<T*, A, B, C, D, E, F, G, H, I, J>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I, J, K> wrapper = Wrap(&_New11<T, A, B, C, D, E, F, G, H, I, J, K>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I, J, K>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I, J, K>(wrapper, a, b, c, d, e, f, g, h, i, j, k), &TBinder<T*, A, B, C, D, E, F, G, H, I, J, K>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I, J, K, L> wrapper = Wrap(&_New12<T, A, B, C, D, E, F, G, H, I, J, K, L>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L>(wrapper, a, b, c, d, e, f, g, h, i, j, k, l), &TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I, J, K, L, M> wrapper = Wrap(&_New13<T, A, B, C, D, E, F, G, H, I, J, K, L, M>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M>(wrapper, a, b, c, d, e, f, g, h, i, j, k, l, m), &TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N> wrapper = Wrap(&_New14<T, A, B, C, D, E, F, G, H, I, J, K, L, M, N>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(wrapper, a, b, c, d, e, f, g, h, i, j, k, l, m, n), &TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> wrapper = Wrap(&_New15<T, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(wrapper, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o), &TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>::operator ()));
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
	TWrapper<T*> WrapFactory(TypeTrait<T>, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) {
		TWrapper<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P> wrapper = Wrap(&_New16<T, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>::Invoke);
		return TWrapper<T*>(TMethod<std::true_type, TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>, T*>(new TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>(wrapper, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p), &TBinder<T*, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P>::operator ()));
	}
#else

	// Wraps arguments passed into TWrapper<> call. Just as std::bind.
	template <typename R, typename... Args>
	class TBinder {
	public:
		TBinder() {}
	
		template <typename... Params>
		explicit TBinder(TWrapper<R, Args...>&& f, Params&&... params) : func(std::forward<TWrapper<R, Args...>>(f)), args(std::forward<Params>(params)...) {}
		
		typename ReturnType<R>::type operator () () {
			return Apply(gen_seq<sizeof...(Args)>());
		}
	
	protected:
		template <size_t... I>
		typename ReturnType<R>::type Apply(seq<I...>) {
			return func(std::get<I>(args)...);
		}
	
		TWrapper<R, Args...> func;
		std::tuple<typename std::decay<Args>::type...> args;
	};
	
	template <typename T, typename... Params>
	static T* _New(Params... params) {
		return new T(std::forward<Params>(params)...);
	}
	
	template <typename T, typename... Params>
	static TWrapper<T*> WrapFactory(TypeTrait<T>, Params&&... params) {
		return TWrapper<T*>(TMethod<true, TBinder<T*, Params...>, T*>(new TBinder<T*, Params...>(Wrap(&_New<T, Params...>), std::forward<Params>(params)...), &TBinder<T*, Params...>::operator ()));
	}

#endif
}
