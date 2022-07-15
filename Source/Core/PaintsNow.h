// PaintsNow Core Header
// Provides a fundamental framework via C++ 11 and VC6
// KeWaitForSingleObject(GetCurrentThread(), INFINITE);
// PaintDream (paintdream@paintdream.com)
// 

#pragma once

// PaintsNow.h is to eliminate imcompatibilities between modern and legacy compilers,
// especially for Microsoft Visual C++ 6.0
// Implement static_assert
#if defined(_MSC_VER) && _MSC_VER <= 1200
#define static_assert(f, g) { int check = sizeof(char[2*(int)(f)]); }
#define singleton
#else
#define singleton static
#endif

// If you've configured PaintsNow with CMake, then we include its configuration file
#ifdef CMAKE_PAINTSNOW
#include <PaintsNowConfig.h>
#if defined(NDEBUG)
#if defined(_DEBUG)
#undef _DEBUG
#endif
#else
#if !defined(_DEBUG) && defined(_MSC_VER)
#define _DEBUG
#endif
#endif
#else
// Otherwise fallback to precompiled thirdparty libraries
#define USE_STATIC_THIRDPARTY_LIBRARIES 1
#endif // CMAKE_PAINTSNOW

// Disable CRT function warnings
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

// Rewrite memory allocation with 16 as default alignment.
// So we can use aligned SIMD (SSE+/Neon) freely

#ifndef USE_RAW_MEMORY_ALIGNMENT
#include <cstddef>
#if __STDCPP_DEFAULT_NEW_ALIGNMENT__ < 16
void* operator new (size_t size);
void operator delete (void* ptr);
void* operator new[] (size_t size);
#if defined(_MSC_VER) && _MSC_VER <= 1200
inline void* operator new[] (size_t size, void* ptr) { return ptr; }
#endif
void operator delete[] (void* ptr);
#endif
#endif

// Dynamic linked library support.
#ifdef PAINTSNOW_DLL
// From http://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_DLL
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__ ((dllexport))
#else
#define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__ ((dllimport))
#else
#define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define DLL_LOCAL
#else
#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define DLL_PUBLIC
#define DLL_LOCAL
#endif
#endif
#else
#define DLL_PUBLIC
#define DLL_LOCAL
#endif

namespace PaintsNow {
	// From ones to zeros, the world finally falls into void.
	// Eliminates the difference of member function pointer size between single derivation and multiple derivation of MSVC compiler
	class Zero {};
	class One {};
	class Void : public One, public Zero {};
}

// Most of CPU have 64-byte cacheline.
#define CPU_CACHELINE_SIZE 64
#ifdef _MSC_VER
#if _MSC_VER <= 1200
	#define CPU_LP64 1
#else
	#ifdef _WIN64
		#define CPU_LP64 1
	#else
		#define CPU_LP64 0
	#endif
#endif
#else
	#ifdef __LP64__
		#define CPU_LP64 1
	#else
		#define CPU_LP64 0
	#endif
#endif

#ifdef _MSC_VER
#if _MSC_VER <= 1200
// Disable some annoying warnings of VC6
#pragma warning(disable:4355)
#pragma warning(disable:4503)
#pragma warning(disable:4530)
#pragma warning(disable:4666)
#pragma warning(disable:4786)
#pragma comment(linker, "/ignore:4006")
#pragma comment(linker, "/ignore:4098")
#define __STL_HAS_NAMESPACES
#include <algorithm>
#include <vector>
// defines C++ keywords for VC6
#define override
#define noexcept
#define final
#define constexpr
#define nullptr (0)
#define thread_local __declspec(thread)
#define alignof __alignof
#define emplace_back push_back
#define emplace_front push_front

// Implement rvalue<> for VC6
// Use alignment... anyone has better ideas?
#pragma pack(push, 1)
template <class T>
class rvalue {
public:
	rvalue(T& t) : pointer(&t) {}
	rvalue(const rvalue& v) : pointer(v.pointer) {}
	operator T& () const {
		return *pointer;
	}
	T* pointer;
};
#pragma pack(pop)

#else
#include <utility>
template <class T>
using rvalue = T&&; // for Compilers that support rvalue reference
#endif
#pragma warning(disable:4309)
#pragma warning(disable:4316)
#pragma warning(disable:4819)
#else
#include <utility>
template <class T>
using rvalue = T&&;
#endif

// Formalize use of alignment
#if defined(_MSC_VER) && _MSC_VER <= 1200
#define class_aligned(f) __declspec(align(f)) class
#define struct_aligned(f) __declspec(align(f)) struct
#define alignas(f) __declspec(align(f))
#else
#define class_aligned(f) class alignas(f)
#define struct_aligned(f) struct alignas(f)
#endif

#if defined(_MSC_VER)
#define pure_interface __declspec(novtable)
#define forceinline __forceinline
#else
#define pure_interface
#define forceinline __attribute__((always_inline))
#endif

namespace PaintsNow {
	template <class T>
	class TShared; // Forward declaration for TShared<T> in Tiny.h
	template <class T>
	class TUnique; // Forward declaration for TUnique<T> in Tiny.h
	
	template <class T>
	struct TypeTrait {}; // for function static dispatching
}

// Establish std::remove_xxx/std::is_xxx series functions for VC6

#include <vector>
#if defined(_MSC_VER) && _MSC_VER <= 1200
// std::remove_reference, std::remove_const, std::decay & std::ref for vc6, from boost library
// simplified by sinojelly. http://sinojelly.blog.51cto.com/479153/100307/
//
namespace std {
	template <class T> struct wrap {};
	typedef char true_type;
	struct false_type { char padding[8]; };

	namespace detail {
		using ::std::true_type;
		using ::std::false_type;
		using ::std::wrap;
		template <class T> T&(* is_reference_helper1(wrap<T>) )(wrap<T>);
		char is_reference_helper1(...);
		template <class T> false_type is_reference_helper2(T&(*)(wrap<T>));
		true_type is_reference_helper2(...);

		template <typename T>
		struct is_reference_impl {
			enum { value = sizeof(::std::detail::is_reference_helper2(::std::detail::is_reference_helper1(::std::wrap<T>()))) == 1 };
		};

		template <> struct is_reference_impl<void> { enum { value = false }; };

		template <class T> T (* is_const_helper1(wrap<T>) )(wrap<T>);
		char is_const_helper1(...);
		template <class T> false_type is_const_helper2(T const(*)(wrap<T>));
		true_type is_const_helper2(...);

		template <typename T>
		struct is_const_impl {
			enum { value = sizeof(::std::detail::is_const_helper2(::std::detail::is_const_helper1(::std::wrap<T>()))) != 1 };
		};

		template <> struct is_const_impl<void> { enum { value = false }; };

		template <class T> T (* is_pointer_helper1(wrap<T>) )(wrap<T>);
		char is_pointer_helper1(...);
		template <class T> false_type is_pointer_helper2(T* (*)(wrap<T>));
		true_type is_pointer_helper2(...);

		template <typename T>
		struct is_pointer_impl {
			enum { value = sizeof(::std::detail::is_pointer_helper2(::std::detail::is_pointer_helper1(::std::wrap<T>()))) == 1 };
		};

		template <> struct is_pointer_impl<void> { enum { value = false }; };

		template <class T> T(*is_shared_helper1(wrap<T>))(wrap<T>);
		char is_shared_helper1(...);
		template <class T> false_type is_shared_helper2(PaintsNow::TShared<T>(*)(wrap<T>));
		true_type is_shared_helper2(...);

		template <typename T>
		struct is_shared_impl {
			enum { value = sizeof(::std::detail::is_shared_helper2(::std::detail::is_shared_helper1(::std::wrap<T>()))) == 1 };
		};

		template <> struct is_shared_impl<void> { enum { value = false }; };

		template <class T> T(*is_vector_helper1(wrap<T>))(wrap<T>);
		char is_vector_helper1(...);
		template <class T> false_type is_vector_helper2(vector<T>(*)(wrap<T>));
		true_type is_vector_helper2(...);

		template <typename T>
		struct is_vector_impl {
			enum { value = sizeof(::std::detail::is_vector_helper2(::std::detail::is_vector_helper1(::std::wrap<T>()))) == 1 };
		};

		template <> struct is_vector_impl<void> { enum { value = false }; };

		template <class T> T(*is_rvalue_helper1(wrap<T>))(wrap<T>);
		char is_rvalue_helper1(...);
		template <class T> false_type is_rvalue_helper2(rvalue<T>(*)(wrap<T>));
		true_type is_rvalue_helper2(...);

		template <typename T>
		struct is_rvalue_impl {
			enum { value = sizeof(::std::detail::is_rvalue_helper2(::std::detail::is_rvalue_helper1(::std::wrap<T>()))) == 1 };
		};

		template <> struct is_rvalue_impl<void> { enum { value = false }; };

		// impl
		template <typename ID>
		struct msvc_extract_type { struct id2type; };

		template <typename T, typename ID, typename S>
		struct msvc_register_type : msvc_extract_type<ID> {
			typedef msvc_extract_type<ID> base_type;
			// This uses nice VC6.5 and VC7.1 bugfeature
			struct base_type::id2type { typedef T type; typedef S selector; };
		};

		template <bool IsReference>
		struct remove_reference_impl_typeof {
			template <typename T, typename ID>
			struct inner { typedef T type; };
		};

		template <>
		struct remove_reference_impl_typeof<true> {
			template <typename T, typename ID>
			struct inner {
				template <typename U>
				static msvc_register_type<U, ID, true_type> test(U&(*)());
				static msvc_register_type<T, ID, false_type> test(...);
				enum { register_test = sizeof(test( (T(*)())(nullptr) ) ) };
				typedef typename msvc_extract_type<ID>::id2type::type type;
				typedef typename msvc_extract_type<ID>::id2type::selector selector;
			};
		};

		template <bool IsConst>
		struct remove_const_impl_typeof {
			template <typename T, typename ID>
			struct inner { typedef T type; };
		};
	
		template <>
		struct remove_const_impl_typeof<true> {
			template <typename T, typename ID>
			struct inner {
				template <typename U>
				static msvc_register_type<U, ID, true_type> test(U const(*)());
				static msvc_register_type<T, ID, false_type> test(...);
				enum { register_test = sizeof(test( (T(*)())(nullptr) ) ) };
				typedef typename msvc_extract_type<ID>::id2type::type type;
				typedef typename msvc_extract_type<ID>::id2type::selector selector;
			};
		};

		template <bool IsPointer>
		struct remove_pointer_impl_typeof {
			template <typename T, typename ID>
			struct inner { typedef T type; };
		};

		template <>
		struct remove_pointer_impl_typeof<true> {
			template <typename T, typename ID>
			struct inner {
				template <typename U>
				static msvc_register_type<U, ID, true_type> test(U *(*)());
				static msvc_register_type<T, ID, false_type> test(...);
				enum { register_test = sizeof(test((T(*)())(nullptr))) };
				typedef typename msvc_extract_type<ID>::id2type::type type;
				typedef typename msvc_extract_type<ID>::id2type::selector selector;
			};
		};

		template <bool IsShared>
		struct remove_shared_impl_typeof {
			template <typename T, typename ID>
			struct inner { typedef T type; };
		};

		template <>
		struct remove_shared_impl_typeof<true> {
			template <typename T, typename ID>
			struct inner {
				template <typename U>
				static msvc_register_type<U*, ID, true_type> test(PaintsNow::TShared<U>(*)());
				static msvc_register_type<T, ID, false_type> test(...);
				enum { register_test = sizeof(test((T(*)())(nullptr))) };
				typedef typename msvc_extract_type<ID>::id2type::type type;
				typedef typename msvc_extract_type<ID>::id2type::selector selector;
			};
		};

		template <bool IsVector>
		struct remove_vector_impl_typeof {
			template <typename T, typename ID>
			struct inner { typedef T type; };
		};
		template <>
		struct remove_vector_impl_typeof<true> {
			template <typename T, typename ID>
			struct inner {
				template <typename U>
				static msvc_register_type<U, ID, true_type> test(vector<U> (*)());
				static msvc_register_type<T, ID, false_type> test(...);
				enum { register_test = sizeof(test( (T(*)())(0) ) ) };
				typedef typename msvc_extract_type<ID>::id2type::type type;
				typedef typename msvc_extract_type<ID>::id2type::selector selector;
			};
		};

		template <bool IsRvalue>
		struct remove_rvalue_impl_typeof {
			template <typename T, typename ID>
			struct inner { typedef T type; };
		};
		template <>
		struct remove_rvalue_impl_typeof<true> {
			template <typename T, typename ID>
			struct inner {
				template <typename U>
				static msvc_register_type<U, ID, true_type> test(rvalue<U>(*)());
				static msvc_register_type<T, ID, false_type> test(...);
				enum { register_test = sizeof(test((T(*)())(0))) };
				typedef typename msvc_extract_type<ID>::id2type::type type;
				typedef typename msvc_extract_type<ID>::id2type::selector selector;
			};
		};
	}

	template <typename T>
	struct remove_rvalue {
		typedef typename detail::remove_rvalue_impl_typeof<std::detail::is_rvalue_impl<T>::value>::template inner<T, remove_rvalue<T> >::type type;
	};

	template <typename T>
	struct is_rvalue {
		enum { value = std::detail::is_rvalue_impl<T>::value };
	};

	template <typename T>
	struct remove_reference {
		typedef typename detail::remove_reference_impl_typeof<std::detail::is_reference_impl<T>::value>::template inner<T, remove_reference<T> >::type type;
	};

	template <typename T>
	struct is_reference {
		enum { value = std::detail::is_reference_impl<T>::value };
	};

	template <typename T>
	struct remove_const {
		typedef typename detail::remove_const_impl_typeof<std::detail::is_const_impl<T>::value>::template inner<T, remove_const<T> >::type type;
	};

	template <typename T>
	struct is_const {
		enum { value = std::detail::is_const_impl<std::remove_reference<T>::type>::value };
	};

	template <typename T>
	struct remove_shared {
		typedef typename detail::remove_shared_impl_typeof<std::detail::is_shared_impl<T>::value>::template inner<T, remove_shared<T> >::type type;
	};

	template <typename T>
	struct is_shared {
		enum { value = std::detail::is_shared_impl<T>::value };
	};

	template <typename T>
	struct remove_pointer {
		typedef typename remove_shared<T>::type S;
		typedef typename detail::remove_pointer_impl_typeof<std::detail::is_pointer_impl<S>::value>::template inner<S, remove_pointer<S> >::type type;
	};

	template <typename T>
	struct is_pointer {
		typedef typename remove_shared<T>::type S;
		typedef typename detail::remove_pointer_impl_typeof<std::detail::is_pointer_impl<S>::value>::template inner<S, remove_pointer<S> >::selector type;
		enum { value = std::detail::is_pointer_impl<S>::value };
	};

	template <typename T>
	struct remove_vector {
		typedef typename detail::remove_vector_impl_typeof<std::detail::is_vector_impl<T>::value>::template inner<T,remove_vector<T> >::type type;
	};

	template <typename T>
	struct is_vector {
		typedef typename detail::remove_vector_impl_typeof<std::detail::is_vector_impl<T>::value>::template inner<T,remove_vector<T> >::selector type;
		enum { value = std::detail::is_vector_impl<T>::value };
	};

	template <typename T>
	struct decay {
		typedef typename remove_rvalue<T>::type norvaluetype;
		typedef typename remove_reference<norvaluetype>::type noreftype;
		typedef typename remove_const<noreftype>::type type;
	};

	template <class T>
	struct reference_wrapper {
		reference_wrapper() : pointer(nullptr) {}
		reference_wrapper(T& object) : pointer(&object) {}
		operator T& () {
			return *pointer;
		}

		reference_wrapper& operator = (T& object) {
			pointer = &object;
			return this;
		}

	private:
		T* pointer;
	};

	template <class T>
	reference_wrapper<T> ref(T& object) {
		return reference_wrapper<T>(object);
	}

	template <class T>
	rvalue<remove_rvalue<T>::type> move(T& t) {
		return t;
	}
}

#else

namespace std {
	template <class T>
	struct remove_shared {
		typedef T type;
	};

	template <class T>
	struct remove_shared<PaintsNow::TShared<T>> {
		typedef T* type;
	};

	template <class T>
	struct remove_pointer<PaintsNow::TShared<T>> {
		typedef T type;
	};

	template <class T>
	struct is_pointer<PaintsNow::TShared<T>> : std::true_type {};

	template <class T>
	struct remove_vector {
		typedef T type;
	};

	template <class T>
	struct remove_vector<vector<T>> {
		typedef T type;
	};

	template <class T>
	struct is_vector : public false_type {};

	template <class T>
	struct is_vector<vector<T>> : public true_type {};

	template <class T>
	struct is_movable {
		enum { value = !std::is_const<T>::value && std::is_reference<T>::value && !std::is_rvalue_reference<T>::value };
	};

	template <class T, class F>
	typename std::enable_if<!is_movable<T>::value, typename std::remove_reference<F>::type&&>::type try_move(F&& t) {
		return static_cast<typename std::remove_reference<F>::type&&>(t);
	}

	template <class T, class F>
	typename std::enable_if<is_movable<T>::value, typename std::remove_reference<F>::type&>::type try_move(F&& t) {
		return static_cast<typename std::remove_reference<F>::type&>(t);
	}

	template <class T>
	void destruct(T* ptr) {
		ptr->~T();
	}
}

#endif

// Checked integer cast for overflow checking
#include <cassert>
template <class T>
struct verify_cast {
	template <class D>
	verify_cast(D v) : value(static_cast<T>(v)) {
		assert((D)value == v);
	}

	operator T() const {
		return value;
	}

	T value;
};

template <class D>
struct getboolean { enum { value = false }; };
template <>
struct getboolean<std::true_type> { enum { value = true }; };

#if !(defined(_MSC_VER) && _MSC_VER <= 1200)
#include <tuple>
#include <utility>
#endif

// std::make_index_sequence for C++ 11
// seq from stackoverflow http://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence By Xeo
#if defined(_MSC_VER)
#if _MSC_VER > 1200
	template <unsigned...> struct seq { typedef seq type; };
	template <class S1, class S2> struct concat;
	template <unsigned... I1, unsigned... I2>
	struct concat<seq<I1...>, seq<I2...>>
		: public seq<I1..., (sizeof...(I1)+I2)...> {};
	template <unsigned N> struct gen_seq;
	template <unsigned N>
	struct gen_seq : public concat<typename gen_seq<N / 2>::type, typename gen_seq<N - N / 2>::type>::type {};
	template <> struct gen_seq<0> : seq<> {};
	template <> struct gen_seq<1> : seq<0> {};
#endif
#else
	template <std::size_t... i>
	struct seq {
		typedef std::size_t value_type;
		typedef seq<i...> type;
		// gcc-4.4.7 doesn't support `constexpr` and `noexcept`.
		static /*constexpr*/ std::size_t size() /*noexcept*/ {
			return sizeof ... (i);
		}
	};

	// this structure doubles seq elements.
	// s- is number of template arguments in IS.
	template <std::size_t s, typename IS>
	struct doubled_seq;

	template <std::size_t s, std::size_t... i>
	struct doubled_seq<s, seq<i... > > {
		typedef seq<i..., (s + i)...> type;
	};

	// this structure incremented by one seq, iff NEED-is true,
	// otherwise returns IS
	template <bool NEED, typename IS>
	struct inc_seq;

	template <typename IS>
	struct inc_seq<false, IS> { typedef IS type; };

	template <std::size_t ... i>
	struct inc_seq<true, seq<i...>> {
		typedef seq<i..., sizeof...(i)> type;
	};

	// helper structure for make_seq.
	template <std::size_t N>
	struct make_seq_impl : inc_seq<(N % 2 != 0),
					typename doubled_seq<N / 2,
					typename make_seq_impl<N / 2>::type
				>::type> {};

	 // helper structure needs specialization only with 0 element.
	template <> struct make_seq_impl<0> { typedef seq<> type; };

	// OUR make_seq,  gcc-4.4.7 doesn't support `using`,
	// so we use struct instead of it.
	template <std::size_t N>
	struct gen_seq : make_seq_impl<N>::type {};
#endif

