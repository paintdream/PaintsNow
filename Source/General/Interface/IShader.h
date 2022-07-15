// IShader.h -- shader interface
// PaintDream (paintdream@paintdream.com)
// 2014-12-3
//

#pragma once

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IReflect.h"
#include "IRender.h"
#include <string>
#include <map>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4700) // local variable '$' used without having been initialized
#endif

namespace PaintsNow {
	class pure_interface IShader : public IReflectObjectComplex {
	public:
		virtual String GetShaderText();
		virtual String GetPredefines();

		static UInt3 WorkGroupSize;
		static UInt3 NumWorkGroups;
		static UInt3 LocalInvocationID;
		static UInt3 WorkGroupID;
		static UInt3 GlobalInvocationID;
		static uint32_t LocalInvocationIndex;

		class MetaShader : public TReflected<MetaShader, MetaNodeBase> {
		public:
			MetaShader(IRender::Resource::ShaderDescription::Stage shaderType);
			MetaShader operator = (IRender::Resource::ShaderDescription::Stage shaderType);

			template <class T, class D>
			inline const MetaShader& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef MetaShader Type;
			};

			typedef MetaShader Type;

			TObject<IReflect>& operator () (IReflect& reflect) override;
			IRender::Resource::ShaderDescription::Stage shaderType;
		};

		template <class M>
		class TMetaBinding : public TReflected<TMetaBinding<M>, MetaNodeBase> {
		public:
			typedef TReflected<TMetaBinding<M>, MetaNodeBase> BaseClass;
			TMetaBinding(const M& m = M()) : description(m) {}

			template <class T, class D>
			inline const TMetaBinding& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef TMetaBinding Type;
			};

			typedef TMetaBinding Type;

			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);

				if (reflect.IsReflectProperty()) {
					ReflectProperty(description)[Runtime];
				}

				return *this;
			}

			M description;
		};

		enum MEMORY_SPEC {
			MEMORY_DEFAULT,
			MEMORY_COHERENT,
			MEMORY_VOLATILE,
			MEMORY_RESTRICT,
			MEMORY_READONLY,
			MEMORY_WRITEONLY
		};

		template <class M>
		class TMetaBindingResource : public TReflected<TMetaBindingResource<M>, TMetaBinding<M> > {
		public:
			TMetaBindingResource() : resource(nullptr), memorySpec(MEMORY_DEFAULT) {}

			operator bool() {
				return resource != nullptr;
			}

			IRender::Resource* resource;
			MEMORY_SPEC memorySpec;
		};

		template <class M>
		class BindConst : public TReflected<BindConst<M>, TMetaBinding<M> > {
		public:
			typedef TReflected<BindConst<M>, TMetaBinding<M> > BaseClass;
			BindConst(const M& m = 0) : BaseClass(m) {}

			template <class T, class D>
			inline const BindConst& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindConst Type;
			};

			typedef BindConst Type;
		};

		class BindEnable : public TReflected<BindEnable, TMetaBinding<bool*> > {
		public:
			BindEnable(bool& value) { description = &value; }
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindEnable& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindEnable Type;
			};

			typedef BindEnable Type;
		};

		class BindInput : public TReflected<BindInput, TMetaBinding<uint32_t> > {
		public:
			enum SCHEMA {
				GENERAL, COMPUTE_GROUP, LOCAL,
				INDEX, POSITION, RASTERCOORD, NORMAL, BINORMAL, TANGENT, COLOR, COLOR_INSTANCED, BONE_INDEX, BONE_WEIGHT,
				TRANSFORM_WORLD, TRANSFORM_WORLD_NORMAL, BONE_TRANSFORMS, TRANSFORM_VIEW, 
				TRANSFORM_VIEWPROJECTION, TRANSFORM_VIEWPROJECTION_INV, TRANSFORM_LAST_VIEWPROJECTION, 
				LIGHT, UNITCOORD, MAINTEXTURE, TEXCOORD };

			BindInput(uint32_t t = GENERAL, const TWrapper<UInt2>& q = nullptr) : BaseClass(t), subRangeQueryer(q) {}
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindInput& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindInput Type;
			};

			typedef BindInput Type;
			TWrapper<UInt2> subRangeQueryer;
		};

		class BindOutput : public TReflected<BindOutput, TMetaBinding<uint32_t> > {
		public:
			enum SCHEMA { GENERAL, LOCAL, HPOSITION, DEPTH, COLOR, TEXCOORD };
			BindOutput(uint32_t t = GENERAL) : BaseClass(t) {}
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindOutput& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindOutput Type;
			};

			typedef BindOutput Type;
		};

		class BindTexture : public TReflected<BindTexture, TMetaBindingResource<IRender::Resource::TextureDescription> > {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindTexture& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindTexture Type;
			};

			typedef BindTexture Type;
		};

		class BindBuffer : public TReflected<BindBuffer, TMetaBindingResource<IRender::Resource::BufferDescription> > {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindBuffer& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindBuffer Type;
			};

			typedef BindBuffer Type;
		};

		class BindFunction : public TReflected<BindFunction, MetaNodeBase> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

#if defined(_MSC_VER) && _MSC_VER <= 1200
			template <class T>
			static String GetCode(T* t, String(T::* pfn)()) {
				return (t->*pfn)();
			}

			template <class T, class A>
			static String GetCode(T* t, String(T::* pfn)(A)) {
				std::decay<A>::type a;

				return (t->*pfn)(a);
			}

			template <class T, class A, class B>
			static String GetCode(T* t, String(T::* pfn)(A, B)) {
				std::decay<A>::type a;
				std::decay<B>::type b;

				return (t->*pfn)(a, b);
			}

			template <class T, class A, class B, class C>
			static String GetCode(T* t, String(T::* pfn)(A, B, C)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;

				return (t->*pfn)(a, b, c);
			}

			template <class T, class A, class B, class C, class D>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;

				return (t->*pfn)(a, b, c, d);
			}

			template <class T, class A, class B, class C, class D, class E>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;

				return (t->*pfn)(a, b, c, d, e);
			}

			template <class T, class A, class B, class C, class D, class E, class F>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;

				return (t->*pfn)(a, b, c, d, e, f);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;

				return (t->*pfn)(a, b, c, d, e, f, g);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;

				return (t->*pfn)(a, b, c, d, e, f, g, h);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I, J)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;
				std::decay<J>::type j;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i, j);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I, J, K)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;
				std::decay<J>::type j;
				std::decay<K>::type k;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i, j, k);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I, J, K, L)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;
				std::decay<J>::type j;
				std::decay<K>::type k;
				std::decay<L>::type l;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i, j, k, l);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I, J, K, L, M)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;
				std::decay<J>::type j;
				std::decay<K>::type k;
				std::decay<L>::type l;
				std::decay<M>::type m;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i, j, k, l, m);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I, J, K, L, M, N)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;
				std::decay<J>::type j;
				std::decay<K>::type k;
				std::decay<L>::type l;
				std::decay<M>::type m;
				std::decay<N>::type n;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;
				std::decay<J>::type j;
				std::decay<K>::type k;
				std::decay<L>::type l;
				std::decay<M>::type m;
				std::decay<N>::type n;
				std::decay<O>::type o;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
			}

			template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
			static String GetCode(T* t, String(T::* pfn)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P)) {
				std::decay<A>::type a;
				std::decay<B>::type b;
				std::decay<C>::type c;
				std::decay<D>::type d;
				std::decay<E>::type e;
				std::decay<F>::type f;
				std::decay<G>::type g;
				std::decay<H>::type h;
				std::decay<I>::type i;
				std::decay<J>::type j;
				std::decay<K>::type k;
				std::decay<L>::type l;
				std::decay<M>::type m;
				std::decay<N>::type n;
				std::decay<O>::type o;
				std::decay<P>::type p;

				return (t->*pfn)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
			}
#else
			template <typename T, typename G, typename... Args, size_t... I>
			static String Apply(T* t, G& args, String(T::* pfn)(Args...), seq<I...>) {
				return (t->*pfn)(std::try_move<typename std::tuple_element<I, std::tuple<Args...>>::type>(std::get<I>(args))...);
			}

			template <typename T, typename... Args>
			static String GetCode(T* t, String(T::* pfn)(Args...)) {
				std::tuple<typename std::decay<Args>::type...> args;
				return Apply(t, args, pfn, gen_seq<sizeof...(Args)>());
			}
#endif

			template <class T, class D>
			inline BindFunction FilterField(T* t, D* d) const {
				BindFunction func = *this;
				func.codeString = GetCode(t, *d);
				return func;
			}

			template <class T, class D>
			struct RealType {
				typedef BindFunction Type;
			};

			typedef BindFunction Type;
			String codeString;
		};
	};
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
