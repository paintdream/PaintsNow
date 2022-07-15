// TVector -- Basic abstract template vector
// PaintDream (paintdream@paintdream.com)
// 2014-11-30
//

#pragma once

#include "../PaintsNow.h"
#include "TAlgorithm.h"
#include <cstring>
#include <cmath>
#include <algorithm>

#if defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_IX86))
#define USE_SSE
#if _MSC_VER > 1200
#define USE_SSE_LATEST
#endif
#else
#if defined(__i386__) || defined(__x86_64__)
#define USE_SSE
#endif
#endif

#ifdef USE_SSE
#ifdef USE_SSE_LATEST
#include <immintrin.h>
#else
#include <emmintrin.h>
#endif
#endif

namespace PaintsNow {
	// Fixed-size vector
	template <class T, size_t n>
	struct TVectorBase {
		T data[n];
	};

#ifdef USE_SSE
	template <>
	struct_aligned(16) TVectorBase<float, 4U> {
		float data[4];
	};
#endif

	template <class T, size_t n>
	struct TVector : public TVectorBase<T, n> {
		typedef T type;
// #if !defined(_MSC_VER) || _MSC_VER <= 1200
		using TVectorBase<T, n>::data;
// #endif

		inline TVector() {}
		// Construct from half pair
		/*
		inline TVector(const std::pair<TVector<T, n / 2>, TVector<T, n / 2> >& p) {
			memcpy(data, &p.first.data[0], n * sizeof(T) / 2);
			memcpy(data + n / 2, &p.second.data[0], n * sizeof(T) / 2);
		}*/

		// Construct from element scalar
		inline TVector(const T& v) {
			for (size_t i = 0; i < n; i++) {
				data[i] = v;
			}
		}

		enum { size = n };
		inline operator const T* () const {
			return data;
		}

		inline operator T* () {
			return data;
		}

		template <class D>
		inline const T operator [] (D d) const {
			return data[d];
		}

		template <class D>
		inline T& operator [] (D d) {
			return data[d];
		}

		template <class D>
		inline TVector<T, n>& Foreach(D op) {
			for (size_t i = 0; i < n; i++) {
				op(data[i]);
			}

			return *this;
		}

		inline TVector<T, n> operator - () const {
			TVector<T, n> ret;
			for (size_t i = 0; i < n; i++) {
				ret[i] = -data[i];
			}

			return ret;
		}
	};

#ifdef USE_SSE
	inline __m128 LoadVector4f(const TVector<float, 4>& value) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
		return _mm_loadu_ps(&value.data[0]);
#else
		return *(const __m128*)&value.data[0];
#endif
	}

	inline TVector<float, 4> StoreVector4f(__m128 v) {
		return *(const TVector<float, 4>*)&v;
	}
#endif

#define VISIT(X, index) \
	inline const T X() const { return (*this)[index]; } \
	inline T& X() { return (*this)[index]; }

// Fake visit, just make compiler happy
#define SWIZZLE2(X, Y) \
	inline TType2<T> _##X##Y() const { return TType2<T>(X(), Y()); } \

#define SWIZZLE3(X, Y, Z) \
	inline TType3<T> _##X##Y##Z() const { return TType3<T>(X(), Y(), Z()); } \

#define SWIZZLE4(X, Y, Z, W) \
	inline TType4<T> _##X##Y##Z##W() const { return TType4<T>(X(), Y(), Z(), W()); } \

#define SWIZZLE4_FULL(X, Y, Z, W) \
	inline TType4<T> _##X##X##X##X() const { return TType4<T>(X(), X(), X(), X()); } \
	inline TType4<T> _##X##X##X##Y() const { return TType4<T>(X(), X(), X(), Y()); } \
	inline TType4<T> _##X##X##X##Z() const { return TType4<T>(X(), X(), X(), Z()); } \
	inline TType4<T> _##X##X##X##W() const { return TType4<T>(X(), X(), X(), W()); } \
	\
	inline TType4<T> _##X##X##Y##X() const { return TType4<T>(X(), X(), Y(), X()); } \
	inline TType4<T> _##X##X##Y##Y() const { return TType4<T>(X(), X(), Y(), Y()); } \
	inline TType4<T> _##X##X##Y##Z() const { return TType4<T>(X(), X(), Y(), Z()); } \
	inline TType4<T> _##X##X##Y##W() const { return TType4<T>(X(), X(), Y(), W()); } \
	\
	inline TType4<T> _##X##X##Z##X() const { return TType4<T>(X(), X(), Z(), X()); } \
	inline TType4<T> _##X##X##Z##Y() const { return TType4<T>(X(), X(), Z(), Y()); } \
	inline TType4<T> _##X##X##Z##Z() const { return TType4<T>(X(), X(), Z(), Z()); } \
	inline TType4<T> _##X##X##Z##W() const { return TType4<T>(X(), X(), Z(), W()); } \
	\
	inline TType4<T> _##X##X##W##X() const { return TType4<T>(X(), X(), W(), X()); } \
	inline TType4<T> _##X##X##W##Y() const { return TType4<T>(X(), X(), W(), Y()); } \
	inline TType4<T> _##X##X##W##Z() const { return TType4<T>(X(), X(), W(), Z()); } \
	inline TType4<T> _##X##X##W##W() const { return TType4<T>(X(), X(), W(), W()); } \
	\
	\
	inline TType4<T> _##X##Y##X##X() const { return TType4<T>(X(), Y(), X(), X()); } \
	inline TType4<T> _##X##Y##X##Y() const { return TType4<T>(X(), Y(), X(), Y()); } \
	inline TType4<T> _##X##Y##X##Z() const { return TType4<T>(X(), Y(), X(), Z()); } \
	inline TType4<T> _##X##Y##X##W() const { return TType4<T>(X(), Y(), X(), W()); } \
	\
	inline TType4<T> _##X##Y##Y##X() const { return TType4<T>(X(), Y(), Y(), X()); } \
	inline TType4<T> _##X##Y##Y##Y() const { return TType4<T>(X(), Y(), Y(), Y()); } \
	inline TType4<T> _##X##Y##Y##Z() const { return TType4<T>(X(), Y(), Y(), Z()); } \
	inline TType4<T> _##X##Y##Y##W() const { return TType4<T>(X(), Y(), Y(), W()); } \
	\
	inline TType4<T> _##X##Y##Z##X() const { return TType4<T>(X(), Y(), Z(), X()); } \
	inline TType4<T> _##X##Y##Z##Y() const { return TType4<T>(X(), Y(), Z(), Y()); } \
	inline TType4<T> _##X##Y##Z##Z() const { return TType4<T>(X(), Y(), Z(), Z()); } \
	inline TType4<T> _##X##Y##Z##W() const { return TType4<T>(X(), Y(), Z(), W()); } \
	\
	inline TType4<T> _##X##Y##W##X() const { return TType4<T>(X(), Y(), W(), X()); } \
	inline TType4<T> _##X##Y##W##Y() const { return TType4<T>(X(), Y(), W(), Y()); } \
	inline TType4<T> _##X##Y##W##Z() const { return TType4<T>(X(), Y(), W(), Z()); } \
	inline TType4<T> _##X##Y##W##W() const { return TType4<T>(X(), Y(), W(), W()); } \
	\
	\
	inline TType4<T> _##X##Z##X##X() const { return TType4<T>(X(), Z(), X(), X()); } \
	inline TType4<T> _##X##Z##X##Y() const { return TType4<T>(X(), Z(), X(), Y()); } \
	inline TType4<T> _##X##Z##X##Z() const { return TType4<T>(X(), Z(), X(), Z()); } \
	inline TType4<T> _##X##Z##X##W() const { return TType4<T>(X(), Z(), X(), W()); } \
	\
	inline TType4<T> _##X##Z##Y##X() const { return TType4<T>(X(), Z(), Y(), X()); } \
	inline TType4<T> _##X##Z##Y##Y() const { return TType4<T>(X(), Z(), Y(), Y()); } \
	inline TType4<T> _##X##Z##Y##Z() const { return TType4<T>(X(), Z(), Y(), Z()); } \
	inline TType4<T> _##X##Z##Y##W() const { return TType4<T>(X(), Z(), Y(), W()); } \
	\
	inline TType4<T> _##X##Z##Z##X() const { return TType4<T>(X(), Z(), Z(), X()); } \
	inline TType4<T> _##X##Z##Z##Y() const { return TType4<T>(X(), Z(), Z(), Y()); } \
	inline TType4<T> _##X##Z##Z##Z() const { return TType4<T>(X(), Z(), Z(), Z()); } \
	inline TType4<T> _##X##Z##Z##W() const { return TType4<T>(X(), Z(), Z(), W()); } \
	\
	inline TType4<T> _##X##Z##W##X() const { return TType4<T>(X(), Z(), W(), X()); } \
	inline TType4<T> _##X##Z##W##Y() const { return TType4<T>(X(), Z(), W(), Y()); } \
	inline TType4<T> _##X##Z##W##Z() const { return TType4<T>(X(), Z(), W(), Z()); } \
	inline TType4<T> _##X##Z##W##W() const { return TType4<T>(X(), Z(), W(), W()); } \
	\
	\
	inline TType4<T> _##X##W##X##X() const { return TType4<T>(X(), W(), X(), X()); } \
	inline TType4<T> _##X##W##X##Y() const { return TType4<T>(X(), W(), X(), Y()); } \
	inline TType4<T> _##X##W##X##Z() const { return TType4<T>(X(), W(), X(), Z()); } \
	inline TType4<T> _##X##W##X##W() const { return TType4<T>(X(), W(), X(), W()); } \
	\
	inline TType4<T> _##X##W##Y##X() const { return TType4<T>(X(), W(), Y(), X()); } \
	inline TType4<T> _##X##W##Y##Y() const { return TType4<T>(X(), W(), Y(), Y()); } \
	inline TType4<T> _##X##W##Y##Z() const { return TType4<T>(X(), W(), Y(), Z()); } \
	inline TType4<T> _##X##W##Y##W() const { return TType4<T>(X(), W(), Y(), W()); } \
	\
	inline TType4<T> _##X##W##Z##X() const { return TType4<T>(X(), W(), Z(), X()); } \
	inline TType4<T> _##X##W##Z##Y() const { return TType4<T>(X(), W(), Z(), Y()); } \
	inline TType4<T> _##X##W##Z##Z() const { return TType4<T>(X(), W(), Z(), Z()); } \
	inline TType4<T> _##X##W##Z##W() const { return TType4<T>(X(), W(), Z(), W()); } \
	\
	inline TType4<T> _##X##W##W##X() const { return TType4<T>(X(), W(), W(), X()); } \
	inline TType4<T> _##X##W##W##Y() const { return TType4<T>(X(), W(), W(), Y()); } \
	inline TType4<T> _##X##W##W##Z() const { return TType4<T>(X(), W(), W(), Z()); } \
	inline TType4<T> _##X##W##W##W() const { return TType4<T>(X(), W(), W(), W()); } \
	\

	template <class T>
	struct TType3;

	template <class T>
	struct TType4;

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define EXPLICIT
#else
#define EXPLICIT explicit
#endif

	template <class T>
	struct TType2 : public TVector<T, 2> {
		TType2() {}
		TType2(const std::pair<TVector<T, 1>, TVector<T, 1> >& p) : TVector<T, 2>(p) {}
		TType2(const TVector<T, 2>& v) : TVector<T, 2>(v) {}
		TType2(T xx, T yy) { x() = xx; y() = yy; }
		TType2(T xx) { x() = xx; y() = xx; }

		EXPLICIT operator float() const {
			return x();
		}

		VISIT(x, 0);
		VISIT(y, 1);
		VISIT(s, 0);
		VISIT(t, 1);

#ifdef USE_SWIZZLE
		SWIZZLE2(x, y);
		SWIZZLE2(y, x);

		SWIZZLE3(x, x, x);
		SWIZZLE3(x, x, y);
		SWIZZLE3(x, y, x);
		SWIZZLE3(x, y, y);

		SWIZZLE3(y, x, x);
		SWIZZLE3(y, x, y);
		SWIZZLE3(y, y, x);
		SWIZZLE3(y, y, y);

		SWIZZLE4(x, x, x, x);
		SWIZZLE4(x, x, x, y);
		SWIZZLE4(x, x, y, x);
		SWIZZLE4(x, x, y, y);

		SWIZZLE4(x, y, x, x);
		SWIZZLE4(x, y, x, y);
		SWIZZLE4(x, y, y, x);
		SWIZZLE4(x, y, y, y);

		SWIZZLE4(y, x, x, x);
		SWIZZLE4(y, x, x, y);
		SWIZZLE4(y, x, y, x);
		SWIZZLE4(y, x, y, y);

		SWIZZLE4(y, y, x, x);
		SWIZZLE4(y, y, x, y);
		SWIZZLE4(y, y, y, x);
		SWIZZLE4(y, y, y, y);
#endif
	};

	template <class T>
	struct TType3 : public TVector<T, 3> {
		TType3() {}
		TType3(const TVector<T, 3>& v) : TVector<T, 3>(v) {}
		TType3(T xx) { x() = xx; y() = xx; z() = xx; }
		TType3(T xx, T yy, T zz) { x() = xx; y() = yy; z() = zz; }

		EXPLICIT operator float() const {
			return x();
		}

		EXPLICIT operator TType2<T>() const {
			return TType2<T>(x(), y());
		}

		static inline TType3 Load(T f, T padding = 0) {
			return TType3(f, padding, padding);
		}

		static inline TType3 Load(const TVector<T, 2>& v2, T padding = 0) {
			return TType3(v2[0], v2[1], padding);
		}

		static inline TType3 Load(T f, const TVector<T, 2>& v2) {
			return TType3(f, v2[0], v2[1]);
		}

		VISIT(x, 0);
		VISIT(y, 1);
		VISIT(z, 2);

		VISIT(r, 0);
		VISIT(g, 1);
		VISIT(b, 2);

#ifdef USE_SWIZZLE
		SWIZZLE2(x, x);
		SWIZZLE2(x, y);
		SWIZZLE2(x, z);
		SWIZZLE2(y, x);
		SWIZZLE2(y, y);
		SWIZZLE2(y, z);
		SWIZZLE2(z, x);
		SWIZZLE2(z, y);
		SWIZZLE2(z, z);

		SWIZZLE3(x, x, x);
		SWIZZLE3(x, x, y);
		SWIZZLE3(x, x, z);

		SWIZZLE3(x, y, x);
		SWIZZLE3(x, y, y);
		SWIZZLE3(x, y, z);

		SWIZZLE3(x, z, x);
		SWIZZLE3(x, z, y);
		SWIZZLE3(x, z, z);

		SWIZZLE3(y, x, x);
		SWIZZLE3(y, x, y);
		SWIZZLE3(y, x, z);

		SWIZZLE3(y, y, x);
		SWIZZLE3(y, y, y);
		SWIZZLE3(y, y, z);

		SWIZZLE3(y, z, x);
		SWIZZLE3(y, z, y);
		SWIZZLE3(y, z, z);

		SWIZZLE3(z, x, x);
		SWIZZLE3(z, x, y);
		SWIZZLE3(z, x, z);

		SWIZZLE3(z, y, x);
		SWIZZLE3(z, y, y);
		SWIZZLE3(z, y, z);

		SWIZZLE3(z, z, x);
		SWIZZLE3(z, z, y);
		SWIZZLE3(z, z, z);

		// 4
		SWIZZLE4(x, x, x, x);
		SWIZZLE4(x, x, x, y);
		SWIZZLE4(x, x, x, z);

		SWIZZLE4(x, x, y, x);
		SWIZZLE4(x, x, y, y);
		SWIZZLE4(x, x, y, z);

		SWIZZLE4(x, x, z, x);
		SWIZZLE4(x, x, z, y);
		SWIZZLE4(x, x, z, z);

		SWIZZLE4(x, y, x, x);
		SWIZZLE4(x, y, x, y);
		SWIZZLE4(x, y, x, z);

		SWIZZLE4(x, y, y, x);
		SWIZZLE4(x, y, y, y);
		SWIZZLE4(x, y, y, z);

		SWIZZLE4(x, y, z, x);
		SWIZZLE4(x, y, z, y);
		SWIZZLE4(x, y, z, z);

		SWIZZLE4(x, z, x, x);
		SWIZZLE4(x, z, x, y);
		SWIZZLE4(x, z, x, z);

		SWIZZLE4(x, z, y, x);
		SWIZZLE4(x, z, y, y);
		SWIZZLE4(x, z, y, z);

		SWIZZLE4(x, z, z, x);
		SWIZZLE4(x, z, z, y);
		SWIZZLE4(x, z, z, z);

		SWIZZLE4(y, x, x, x);
		SWIZZLE4(y, x, x, y);
		SWIZZLE4(y, x, x, z);

		SWIZZLE4(y, x, y, x);
		SWIZZLE4(y, x, y, y);
		SWIZZLE4(y, x, y, z);

		SWIZZLE4(y, x, z, x);
		SWIZZLE4(y, x, z, y);
		SWIZZLE4(y, x, z, z);

		SWIZZLE4(y, y, x, x);
		SWIZZLE4(y, y, x, y);
		SWIZZLE4(y, y, x, z);

		SWIZZLE4(y, y, y, x);
		SWIZZLE4(y, y, y, y);
		SWIZZLE4(y, y, y, z);

		SWIZZLE4(y, y, z, x);
		SWIZZLE4(y, y, z, y);
		SWIZZLE4(y, y, z, z);

		SWIZZLE4(y, z, x, x);
		SWIZZLE4(y, z, x, y);
		SWIZZLE4(y, z, x, z);

		SWIZZLE4(y, z, y, x);
		SWIZZLE4(y, z, y, y);
		SWIZZLE4(y, z, y, z);

		SWIZZLE4(y, z, z, x);
		SWIZZLE4(y, z, z, y);
		SWIZZLE4(y, z, z, z);

		SWIZZLE4(z, x, x, x);
		SWIZZLE4(z, x, x, y);
		SWIZZLE4(z, x, x, z);

		SWIZZLE4(z, x, y, x);
		SWIZZLE4(z, x, y, y);
		SWIZZLE4(z, x, y, z);

		SWIZZLE4(z, x, z, x);
		SWIZZLE4(z, x, z, y);
		SWIZZLE4(z, x, z, z);

		SWIZZLE4(z, y, x, x);
		SWIZZLE4(z, y, x, y);
		SWIZZLE4(z, y, x, z);

		SWIZZLE4(z, y, y, x);
		SWIZZLE4(z, y, y, y);
		SWIZZLE4(z, y, y, z);

		SWIZZLE4(z, y, z, x);
		SWIZZLE4(z, y, z, y);
		SWIZZLE4(z, y, z, z);

		SWIZZLE4(z, z, x, x);
		SWIZZLE4(z, z, x, y);
		SWIZZLE4(z, z, x, z);

		SWIZZLE4(z, z, y, x);
		SWIZZLE4(z, z, y, y);
		SWIZZLE4(z, z, y, z);

		SWIZZLE4(z, z, z, x);
		SWIZZLE4(z, z, z, y);
		SWIZZLE4(z, z, z, z);
#endif
	};

	template <class T>
	struct TType4 : public TVector<T, 4> {
		TType4() {}
		// TType4(const std::pair<TVector<T, 2>, TVector<T, 2> >& p) : TVector<T, 4>(p) {}
		TType4(const TVector<T, 4>& v) : TVector<T, 4>(v) {}
		TType4(T xx, T yy, T zz, T ww) { x() = xx; y() = yy; z() = zz; w() = ww; }
		TType4(T xx) { x() = xx; y() = xx; z() = xx; w() = xx; }

		EXPLICIT operator float() const {
			return x();
		}

		EXPLICIT operator TType2<T>() const {
			return TType2<T>(x(), y());
		}
		
		EXPLICIT operator TType3<T>() const {
			return TType3<T>(x(), y(), z());
		}

		static inline TType4 Load(T f, T padding = 0) {
			return TType4(f, padding, padding, padding);
		}

		static inline TType4 Load(const TVector<T, 2>& v2, T padding = 0) {
			return TType4(v2[0], v2[1], padding, padding);
		}

		static inline TType4 Load(T f, const TVector<T, 2>& v2, T padding = 0) {
			return TType4(f, v2[0], v2[1], padding);
		}

		static inline TType4 Load(const TVector<T, 2>& v2, const TVector<T, 2>& v2h) {
			return TType4(v2[0], v2[1], v2h[0], v2h[1]);
		}

		static inline TType4 Load(const TVector<T, 3>& v3, T padding = 0) {
			return TType4(v3[0], v3[1], v3[2], padding);
		}

		static inline TType4 Load(T f, const TVector<T, 3>& v3) {
			return TType4(f, v3[0], v3[1], v3[2]);
		}

		VISIT(x, 0);
		VISIT(y, 1);
		VISIT(z, 2);
		VISIT(w, 3);

		VISIT(r, 0);
		VISIT(g, 1);
		VISIT(b, 2);
		VISIT(a, 3);

#ifdef USE_SWIZZLE
		SWIZZLE2(x, x);
		SWIZZLE2(x, y);
		SWIZZLE2(x, z);
		SWIZZLE2(x, w);

		SWIZZLE2(y, x);
		SWIZZLE2(y, y);
		SWIZZLE2(y, z);
		SWIZZLE2(y, w);

		SWIZZLE2(z, x);
		SWIZZLE2(z, y);
		SWIZZLE2(z, z);
		SWIZZLE2(z, w);

		SWIZZLE2(w, x);
		SWIZZLE2(w, y);
		SWIZZLE2(w, z);
		SWIZZLE2(w, w);

		SWIZZLE3(x, x, x);
		SWIZZLE3(x, x, y);
		SWIZZLE3(x, x, z);
		SWIZZLE3(x, x, w);

		SWIZZLE3(x, y, x);
		SWIZZLE3(x, y, y);
		SWIZZLE3(x, y, z);
		SWIZZLE3(x, y, w);

		SWIZZLE3(x, z, x);
		SWIZZLE3(x, z, y);
		SWIZZLE3(x, z, z);
		SWIZZLE3(x, z, w);

		SWIZZLE3(x, w, x);
		SWIZZLE3(x, w, y);
		SWIZZLE3(x, w, z);
		SWIZZLE3(x, w, w);

		SWIZZLE3(y, x, x);
		SWIZZLE3(y, x, y);
		SWIZZLE3(y, x, z);
		SWIZZLE3(y, x, w);

		SWIZZLE3(y, y, x);
		SWIZZLE3(y, y, y);
		SWIZZLE3(y, y, z);
		SWIZZLE3(y, y, w);

		SWIZZLE3(y, z, x);
		SWIZZLE3(y, z, y);
		SWIZZLE3(y, z, z);
		SWIZZLE3(y, z, w);

		SWIZZLE3(y, w, x);
		SWIZZLE3(y, w, y);
		SWIZZLE3(y, w, z);
		SWIZZLE3(y, w, w);

		SWIZZLE3(z, x, x);
		SWIZZLE3(z, x, y);
		SWIZZLE3(z, x, z);
		SWIZZLE3(z, x, w);

		SWIZZLE3(z, y, x);
		SWIZZLE3(z, y, y);
		SWIZZLE3(z, y, z);
		SWIZZLE3(z, y, w);

		SWIZZLE3(z, z, x);
		SWIZZLE3(z, z, y);
		SWIZZLE3(z, z, z);
		SWIZZLE3(z, z, w);

		SWIZZLE3(z, w, x);
		SWIZZLE3(z, w, y);
		SWIZZLE3(z, w, z);
		SWIZZLE3(z, w, w);

		SWIZZLE3(w, x, x);
		SWIZZLE3(w, x, y);
		SWIZZLE3(w, x, z);
		SWIZZLE3(w, x, w);

		SWIZZLE3(w, y, x);
		SWIZZLE3(w, y, y);
		SWIZZLE3(w, y, z);
		SWIZZLE3(w, y, w);

		SWIZZLE3(w, z, x);
		SWIZZLE3(w, z, y);
		SWIZZLE3(w, z, z);
		SWIZZLE3(w, z, w);

		SWIZZLE3(w, w, x);
		SWIZZLE3(w, w, y);
		SWIZZLE3(w, w, z);
		SWIZZLE3(w, w, w);

		SWIZZLE4_FULL(x, y, z, w);
		SWIZZLE4_FULL(y, x, z, w);
		SWIZZLE4_FULL(z, x, y, w);
		SWIZZLE4_FULL(w, x, y, z);
#endif
	};

	// +
	template <class T, size_t n>
	inline TVector<T, n> operator + (const TVector<T, n>& lhs, const T& rhs) {
		TVector<T, n> v = lhs;
		for (size_t i = 0; i < n; i++) {
			v[i] += rhs;
		}

		return v;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
	inline TVector<float, 4> operator + (const TVector<float, 4>& lhs, float rhs) {
#else
	inline TVector<float, 4> operator + (const TVector<float, 4>& lhs, const float& rhs) {
#endif
		__m128 vv = _mm_set_ps1(rhs);
		return StoreVector4f(_mm_add_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n> operator + (const TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		TVector<T, n> res(lhs);
		res += rhs;
		return res;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4> operator + (const TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return StoreVector4f(_mm_add_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator += (TVector<T, n>& lhs, T t) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] += t;
		}
		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4>& operator += (TVector<float, 4>& lhs, float t) {
		__m128 vv = _mm_set_ps1(t);
		return lhs = StoreVector4f(_mm_add_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator += (TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] += rhs[i];
		}
		
		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4>& operator += (TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return lhs = StoreVector4f(_mm_add_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif
	
	// -
	template <class T, size_t n>
	inline TVector<T, n> operator - (const TVector<T, n>& lhs, const T& rhs) {
		TVector<T, n> v = lhs;
		for (size_t i = 0; i < n; i++) {
			v[i] -= rhs;
		}

		return v;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
	inline TVector<float, 4> operator - (const TVector<float, 4>& lhs, float rhs) {
#else
	inline TVector<float, 4> operator - (const TVector<float, 4>& lhs, const float& rhs) {
#endif
		__m128 vv = _mm_set_ps1(rhs);
		return StoreVector4f(_mm_sub_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n> operator - (const TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		TVector<T, n> res(lhs);
		res -= rhs;
		return res;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4> operator - (const TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return StoreVector4f(_mm_sub_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator -= (TVector<T, n>& lhs, T t) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] -= t;
		}
		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4>& operator -= (TVector<float, 4>& lhs, float t) {
		__m128 vv = _mm_set_ps1(t);
		return lhs = StoreVector4f(_mm_sub_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator -= (TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] -= rhs[i];
		}

		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4>& operator -= (TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return lhs = StoreVector4f(_mm_sub_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif

	// *
	template <class T, size_t n>
	inline TVector<T, n> operator * (const TVector<T, n>& lhs, const T& rhs) {
		TVector<T, n> v = lhs;
		for (size_t i = 0; i < n; i++) {
			v[i] *= rhs;
		}

		return v;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
	inline TVector<float, 4> operator * (const TVector<float, 4>& lhs, float rhs) {
#else
	inline TVector<float, 4> operator * (const TVector<float, 4>& lhs, const float& rhs) {
#endif
		__m128 vv = _mm_set_ps1(rhs);
		return StoreVector4f(_mm_mul_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n> operator * (const TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		TVector<T, n> res(lhs);
		res *= rhs;
		return res;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4> operator * (const TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return StoreVector4f(_mm_mul_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator *= (TVector<T, n>& lhs, const T& t) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] *= t;
		}
		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
	inline TVector<float, 4>& operator *= (TVector<float, 4>& lhs, float t) {
#else
	inline TVector<float, 4>& operator *= (TVector<float, 4>& lhs, const float& t) {
#endif
		__m128 vv = _mm_set_ps1(t);
		return lhs = StoreVector4f(_mm_mul_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator *= (TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] *= rhs[i];
		}

		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4>& operator *= (TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return lhs = StoreVector4f(_mm_mul_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif

	// /
	template <class T, size_t n>
	inline TVector<T, n> operator / (const TVector<T, n>& lhs, const T& rhs) {
		TVector<T, n> v = lhs;
		for (size_t i = 0; i < n; i++) {
			v[i] /= rhs;
		}

		return v;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
	inline TVector<float, 4> operator / (const TVector<float, 4>& lhs, float rhs) {
#else
	inline TVector<float, 4> operator / (const TVector<float, 4>& lhs, const float& rhs) {
#endif
		__m128 vv = _mm_set_ps1(rhs);
		return StoreVector4f(_mm_div_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n> operator / (const TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		TVector<T, n> res(lhs);
		res /= rhs;
		return res;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4> operator / (const TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return StoreVector4f(_mm_div_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator /= (TVector<T, n>& lhs, const T& t) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] /= t;
		}
		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
	inline TVector<float, 4>& operator /= (TVector<float, 4>& lhs, float t) {
#else
	inline TVector<float, 4>& operator /= (TVector<float, 4>& lhs, const float& t) {
#endif
		__m128 vv = _mm_set_ps1(t);
		return lhs = StoreVector4f(_mm_div_ps(LoadVector4f(lhs), vv));
	}
#endif

	template <class T, size_t n>
	inline TVector<T, n>& operator /= (TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		for (size_t i = 0; i < n; i++) {
			lhs[i] /= rhs[i];
		}

		return lhs;
	}

#ifdef USE_SSE
#ifndef _MSC_VER
	template <>
#endif
	inline TVector<float, 4>& operator /= (TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
		return lhs = StoreVector4f(_mm_div_ps(LoadVector4f(lhs), LoadVector4f(rhs)));
	}
#endif

	template <class T, size_t n>
	inline bool operator == (const TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
	}

	template <class T, size_t n>
	inline bool operator != (const TVector<T, n>& lhs, const TVector<T, n>& rhs) {
		return memcmp(&lhs, &rhs, sizeof(lhs)) != 0;
	}

	namespace Math {
#if defined(_MSC_VER) && _MSC_VER <= 1200
		template <class T, size_t n>
		void ExtendVector(TVector<TVector<T, 4>, n>& result, const TVector<T, n>& v) {
			for (size_t i = 0; i < 4; i++) {
				for (size_t j = 0; j < n; j++) {
					result[j][i] = v[j];
				}
			}
		}
#else
		template <class T, size_t k, size_t n>
		void ExtendVector(TVector<TVector<T, k>, n>& result, const TVector<T, n>& v) {
			for (size_t i = 0; i < k; i++) {
				for (size_t j = 0; j < n; j++) {
					result[j][i] = v[j];
				}
			}
		}
#endif

		template <class T, size_t n>
		inline T DotProduct(const TVector<T, n>& lhs, const TVector<T, n>& rhs) {
			T res = lhs[0] * rhs[0];
			for (size_t i = 1; i < n; i++) {
				res += lhs[i] * rhs[i];
			}

			return res;
		}

#ifdef USE_SSE
		inline __m128 _Dot(__m128 a, __m128 b) {
#ifdef USE_SSE_LATEST
			return _mm_dp_ps(a, b, 0xff);
#else
			// SSE2 only by now
			__m128 mul0 = _mm_mul_ps(a, b);
			__m128 swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
			__m128 add0 = _mm_add_ps(mul0, swp0);
			__m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(0, 1, 2, 3));
			__m128 add1 = _mm_add_ps(add0, swp1);

			return add1;
#endif
		}

		template <>
		inline float DotProduct(const TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
			TVector<float, 4> ret = StoreVector4f(_Dot(LoadVector4f(lhs), LoadVector4f(rhs)));
			return ret.data[0];
		}
#endif

		template <class T, size_t n>
		inline T SquareLength(const TVector<T, n>& lhs) {
			return DotProduct(lhs, lhs);
		}

#ifdef USE_SSE
		template <>
		inline float SquareLength<float, 4>(const TVector<float, 4>& lhs) {
			__m128 v = LoadVector4f(lhs);
			__m128 m = _Dot(v, v);
			return StoreVector4f(m).data[0];
		}
#endif

		template <class T, size_t n>
		inline T Length(const TVector<T, n>& lhs) {
			return (T)sqrt(SquareLength(lhs));
		}

		template <class T, size_t n>
		inline TVector<T, n> Normalize(const TVector<T, n>& lhs) {
			return lhs / (T)sqrt(SquareLength(lhs));
		}

#ifdef USE_SSE
		template <>
		inline TVector<float, 4> Normalize<float, 4>(const TVector<float, 4>& lhs) {
			__m128 v = LoadVector4f(lhs);
			__m128 dot0 = _Dot(v, v);
			__m128 isr0 = _mm_rsqrt_ps(dot0);
			__m128 mul0 = _mm_mul_ps(v, isr0);

			return StoreVector4f(mul0);
		}
#endif

		template <class T, size_t n>
		inline TVector<T, n> Abs(const TVector<T, n>& lhs) {
			TVector<T, n> ret;
			for (size_t i = 0; i < n; i++) {
				ret[i] = (T)fabs(lhs[i]);
			}

			return ret;
		}

#ifdef USE_SSE
		template <>
		inline TVector<float, 4> Abs(const TVector<float, 4>& lhs) {
#ifdef USE_SSE_LATEST
			__m128 v = _mm_and_ps(LoadVector4f(lhs), _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)));
			return StoreVector4f(v);
#else
			TVector<float, 4> ret;
			ret[0] = fabs(lhs[0]);
			ret[1] = fabs(lhs[1]);
			ret[2] = fabs(lhs[2]);
			ret[3] = fabs(lhs[3]);
			return ret;
#endif
		}
#endif

		template <class T, size_t n>
		inline T ReciprocalLength(const TVector<T, n>& lhs) {
			return T(1) / Length(lhs, lhs);
		}

#ifdef USE_SSE
		template <>
		inline float ReciprocalLength<float, 4>(const TVector<float, 4>& lhs) {
			__m128 v = LoadVector4f(lhs);
			__m128 m = _Dot(v, v);
			__m128 i = _mm_rsqrt_ps(m);
			return StoreVector4f(i).data[0];
		}
#endif

		template <class T>
		inline TVector<T, 3> CrossProduct(const TVector<T, 3>& lhs, const TVector<T, 3>& rhs) {
			TVector<T, 3> t;
			t[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
			t[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
			t[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];

			return t;
		}

		// same as TVector<T, 3>
		template <class T>
		inline TVector<T, 4> CrossProduct(const TVector<T, 4>& lhs, const TVector<T, 4>& rhs) {
			TVector<T, 4> t;
			t[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
			t[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
			t[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
			t[3] = 0;

			return t;
		}

#ifdef USE_SSE
		template <>
		inline TVector<float, 4> CrossProduct(const TVector<float, 4>& lhs, const TVector<float, 4>& rhs) {
			__m128 v1 = LoadVector4f(lhs);
			__m128 v2 = LoadVector4f(rhs);
			__m128 swp0 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 0, 2, 1));
			__m128 swp1 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 1, 0, 2));
			__m128 swp2 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(3, 0, 2, 1));
			__m128 swp3 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(3, 1, 0, 2));
			__m128 mul0 = _mm_mul_ps(swp0, swp3);
			__m128 mul1 = _mm_mul_ps(swp1, swp2);
			__m128 sub0 = _mm_sub_ps(mul0, mul1);

			return StoreVector4f(sub0);
		}
#endif

		template <class T>
		inline bool CrossPoint(TVector<T, 2>& result, const TVector<T, 2>& ps, const TVector<T, 2>& pt, const TVector<T, 2>& qs, const TVector<T, 2>& qt, T& alpha, T& beta) {
			T z = CrossProduct(pt - ps, qt - qs);
			if (fabs(z) < 1e-10)
				return false;

			T A1 = pt[0] - ps[0], A2 = qs[0] - qt[0];
			T B1 = pt[1] - ps[1], B2 = qs[1] - qt[1];
			T C1 = qs[0] - ps[0];
			T C2 = qs[1] - ps[1];
			T D = A1 * B2 - A2 * B1;

			alpha = (B1 * C2 - B2 * C1) / D;
			beta = (C1 * A2 - C2 * A1) / D;

			result = ((ps + (pt - ps) * alpha) + (qs + (qt - qs) * beta)) / T(2);

			return true;
		}

		template <class T>
		inline bool Clip(std::pair<T, T>& lhs, const std::pair<T, T>& rhs) {
			bool b = true;
			for (size_t i = 0; i < T::size; i++) {
				lhs.first[i] = Math::Max(lhs.first[i], rhs.first[i]);
				lhs.second[i] = Math::Min(lhs.second[i], rhs.second[i]);

				if (lhs.first[i] > lhs.second[i])
					b = false;
			}

			return b;
		}

		template <class T>
		inline std::pair<T, T>& Merge(std::pair<T, T>& host, const std::pair<T, T>& rhs) {
			Union(host, rhs.first);
			Union(host, rhs.second);
			return host;
		}

		template <class T>
		inline std::pair<T, T>& Union(std::pair<T, T>& host, const T& value) {
			for (size_t i = 0; i < T::size; i++) {
				host.first[i] = Math::Min(host.first[i], value[i]);
				host.second[i] = Math::Max(host.second[i], value[i]);
			}

			return host;
		}

		template <class T>
		inline bool Overlap(const std::pair<T, T>& lhs, const std::pair<T, T>& rhs) {
			for (size_t i = 0; i < T::size; i++) {
				if (rhs.second[i] < lhs.first[i] || lhs.second[i] < rhs.first[i])
					return false;
			}

			return true;
		}

		template <class T>
		inline bool Contain(const std::pair<T, T>& host, const T& value) {
			for (size_t i = 0; i < T::size; i++) {
				if (value[i] < host.first[i] || host.second[i] < value[i]) {
					return false;
				}
			}

			return true;
		}

		template <class T>
		inline bool Contain(const std::pair<T, T>& host, const std::pair<T, T>& value) {
			return Contain(host, value.first) && Contain(host, value.second);
		}

		template <class T>
		inline T ToLocal(const std::pair<T, T>& val, const T& t) {
			T r;
			for (size_t i = 0; i < T::size; i++) {
				r[i] = (t[i] - val.first[i]) / (val.second[i] - val.first[i]);
			}

			return r;
		}

		template <class T>
		inline T FromLocal(const std::pair<T, T>& val, const T& t) {
			T r;
			for (size_t i = 0; i < T::size; i++) {
				r[i] = (val.second[i] - val.first[i]) * t[i] + val.first[i];
			}

			return r;
		}
	}

	template <class T, class D>
	inline std::pair<T, D> operator * (const std::pair<T, D>& lhs, double t) {
		return std::pair<T, D>(lhs.first * t, lhs.second * t);
	}

	template <class T, class D>
	inline std::pair<T, D> operator / (const std::pair<T, D>& lhs, double t) {
		return std::pair<T, D>(lhs.first / t, lhs.second / t);
	}
}
