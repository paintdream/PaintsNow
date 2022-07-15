// TMatrix.h -- Basic operations on matrix
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once

#include "../PaintsNow.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include "TAlgorithm.h"
#include "TVector.h"

namespace PaintsNow {
	template <class T, size_t m = 4, size_t n = m>
	struct TMatrixBase {
		T data[m][n]; // m: col index, n: row index
	};

#ifdef USE_SSE
	template <>
	struct_aligned(16) TMatrixBase<float, 4U, 4U> {
		float data[4][4]; // m: col index, n: row index
	};
#endif

	template <class T, size_t m = 4, size_t n = m>
	struct TMatrix : public TMatrixBase<T, m, n> {
		enum { M = m, N = n };
		typedef T type;
		// #if !defined(_MSC_VER) || _MSC_VER <= 1200
		using TMatrixBase<T, m, n>::data;
		// #endif

		TMatrix(const T* value) {
			for (size_t i = 0; i < m; i++) {
				for (size_t j = 0; j < n; j++) {
					data[i][j] = *value++;
				}
			}
		}

		template <class D, size_t s, size_t t>
		explicit TMatrix(const TMatrix<D, s, t>& mat) {
			*this = TMatrix<T, m, n>();

			for (size_t i = 0; i < Math::Min(m, s); i++) {
				for (size_t j = 0; j < Math::Min(n, t); j++) {
					data[i][j] = mat.data[i][j];
				}
			}
		}

		static const TMatrix CreateIdentity() {
			TMatrix mat;
			T(*data)[n] = mat.data;
			size_t z = Math::Min(m, n);
			memset(data, 0, sizeof(mat.data));
			for (size_t i = 0; i < z; i++) {
				data[i][i] = 1;
			}

			return mat;
		}

		static const TMatrix& Identity() {
			static TMatrix matrix = CreateIdentity();
			return matrix;
		}

		TMatrix() {}

		TMatrix<T, n, m> Transpose() const {
			TMatrix<T, n, m> target;
			for (size_t i = 0; i < m; i++) {
				for (size_t j = 0; j < n; j++) {
					target(j, i) = (*this)(i, j);
				}
			}

			return target;
		}

		static T Distance(const TMatrix<T, m, n>& lhs, const TMatrix<T, m, n>& rhs) {
			TMatrix<T, m, n> diff;
			TMatrix<T, n, m> star;
			for (size_t i = 0; i < m; i++) {
				for (size_t j = 0; j < n; j++) {
					star(j, i) = diff(i, j) = lhs(i, j) - rhs(i, j);
				}
			}

			TMatrix<T, n, n> result = star * diff;
			T length = 0;
			for (size_t k = 0; k < n; k++) {
				length += result(k, k);
			}

			return (T)sqrt(length);
		}

		operator T* () {
			return data;
		}

		bool operator == (const TMatrix& rhs) const {
			for (size_t i = 0; i < m; i++) {
				for (size_t j = 0; j < n; j++) {
					if (data[i][j] != rhs.data[i][j])
						return false;
				}
			}

			return true;
		}

		bool operator != (const TMatrix& rhs) const {
			return !(*this == rhs);
		}

		TVector<T, n>& operator () (size_t i) {
			return *reinterpret_cast<TVector<T, n>*>(&data[i][0]);
		}

		const TVector<T, n>& operator () (size_t i) const {
			return *reinterpret_cast<const TVector<T, n>*>(&data[i][0]);
		}

		T& operator () (size_t i, size_t j) {
			return data[i][j];
		}

		const T& operator () (size_t i, size_t j) const {
			return data[i][j];
		}
	};

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <class T, size_t m, size_t n>
	TMatrix<T, m, m> operator * (const TMatrix<T, m, n>& lhs, const TMatrix<T, n, m>& rhs) {
		TMatrix<T, m, m> ret;
		for (size_t i = 0; i < m; i++) {
			for (size_t j = 0; j < m; j++) {
				T sum = lhs(i, 0) * rhs(0, j);
				for (size_t k = 1; k < n; k++) {
					sum += lhs(i, k) * rhs(k, j);
				}

				ret(i, j) = sum;
			}
		}

		return ret;
	}
#else
	template <class T, size_t m, size_t n, size_t p>
	TMatrix<T, m, p> operator * (const TMatrix<T, m, n>& lhs, const TMatrix<T, n, p>& rhs) {
		TMatrix<T, m, p> ret;
		for (size_t i = 0; i < m; i++) {
			for (size_t j = 0; j < p; j++) {
				T sum = lhs(i, 0) * rhs(0, j);
				for (size_t k = 1; k < n; k++) {
					sum += lhs(i, k) * rhs(k, j);
				}

				ret(i, j) = sum;
			}
		}

		return ret;
	}
#endif

#ifdef USE_SSE
	template <>
	inline TMatrix<float, 4, 4> operator * (const TMatrix<float, 4, 4>& lhs, const TMatrix<float, 4, 4>& rhs) {
		// SIMD from glm library
		TMatrix<float, 4, 4> ret;
		__m128 rhs0 = LoadVector4f(rhs(0));
		__m128 rhs1 = LoadVector4f(rhs(1));
		__m128 rhs2 = LoadVector4f(rhs(2));
		__m128 rhs3 = LoadVector4f(rhs(3));

		{
			__m128 e0 = _mm_set_ps1(lhs(0, 0));
			__m128 e1 = _mm_set_ps1(lhs(0, 1));
			__m128 e2 = _mm_set_ps1(lhs(0, 2));
			__m128 e3 = _mm_set_ps1(lhs(0, 3));

			__m128 m0 = _mm_mul_ps(rhs0, e0);
			__m128 m1 = _mm_mul_ps(rhs1, e1);
			__m128 m2 = _mm_mul_ps(rhs2, e2);
			__m128 m3 = _mm_mul_ps(rhs3, e3);

			__m128 a0 = _mm_add_ps(m0, m1);
			__m128 a1 = _mm_add_ps(m2, m3);
			__m128 a2 = _mm_add_ps(a0, a1);

			ret(0) = StoreVector4f(a2);
		}

		{
			__m128 e0 = _mm_set_ps1(lhs(1, 0));
			__m128 e1 = _mm_set_ps1(lhs(1, 1));
			__m128 e2 = _mm_set_ps1(lhs(1, 2));
			__m128 e3 = _mm_set_ps1(lhs(1, 3));

			__m128 m0 = _mm_mul_ps(rhs0, e0);
			__m128 m1 = _mm_mul_ps(rhs1, e1);
			__m128 m2 = _mm_mul_ps(rhs2, e2);
			__m128 m3 = _mm_mul_ps(rhs3, e3);

			__m128 a0 = _mm_add_ps(m0, m1);
			__m128 a1 = _mm_add_ps(m2, m3);
			__m128 a2 = _mm_add_ps(a0, a1);

			ret(1) = StoreVector4f(a2);
		}

		{
			__m128 e0 = _mm_set_ps1(lhs(2, 0));
			__m128 e1 = _mm_set_ps1(lhs(2, 1));
			__m128 e2 = _mm_set_ps1(lhs(2, 2));
			__m128 e3 = _mm_set_ps1(lhs(2, 3));

			__m128 m0 = _mm_mul_ps(rhs0, e0);
			__m128 m1 = _mm_mul_ps(rhs1, e1);
			__m128 m2 = _mm_mul_ps(rhs2, e2);
			__m128 m3 = _mm_mul_ps(rhs3, e3);

			__m128 a0 = _mm_add_ps(m0, m1);
			__m128 a1 = _mm_add_ps(m2, m3);
			__m128 a2 = _mm_add_ps(a0, a1);

			ret(2) = StoreVector4f(a2);
		}

		{
			__m128 e0 = _mm_set_ps1(lhs(3, 0));
			__m128 e1 = _mm_set_ps1(lhs(3, 1));
			__m128 e2 = _mm_set_ps1(lhs(3, 2));
			__m128 e3 = _mm_set_ps1(lhs(3, 3));

			__m128 m0 = _mm_mul_ps(rhs0, e0);
			__m128 m1 = _mm_mul_ps(rhs1, e1);
			__m128 m2 = _mm_mul_ps(rhs2, e2);
			__m128 m3 = _mm_mul_ps(rhs3, e3);

			__m128 a0 = _mm_add_ps(m0, m1);
			__m128 a1 = _mm_add_ps(m2, m3);
			__m128 a2 = _mm_add_ps(a0, a1);

			ret(3) = StoreVector4f(a2);
		}

		return ret;
	}
#endif

	template <class T, size_t m, size_t n>
	TVector<T, n> operator * (const TVector<T, m>& value, const TMatrix<T, m, n>& rhs) {
		TVector<T, n> ret;
		for (size_t i = 0; i < n; i++) {
			ret[i] = value[0] * rhs(0, i);
			for (size_t j = 1; j < m; j++) {
				ret[i] += value[j] * rhs(j, i);
			}
		}

		return ret;
	}

#ifdef USE_SSE
	template <>
	inline TVector<float, 4> operator * (const TVector<float, 4>& value, const TMatrix<float, 4, 4>& rhs) {
		// SIMD from glm library
		__m128 v0 = _mm_set_ps1(value[0]);
		__m128 v1 = _mm_set_ps1(value[1]);
		__m128 v2 = _mm_set_ps1(value[2]);
		__m128 v3 = _mm_set_ps1(value[3]);

		__m128 m0 = _mm_mul_ps(LoadVector4f(rhs(0)), v0);
		__m128 m1 = _mm_mul_ps(LoadVector4f(rhs(1)), v1);
		__m128 m2 = _mm_mul_ps(LoadVector4f(rhs(2)), v2);
		__m128 m3 = _mm_mul_ps(LoadVector4f(rhs(3)), v3);

		__m128 a0 = _mm_add_ps(m0, m1);
		__m128 a1 = _mm_add_ps(m2, m3);
		__m128 a2 = _mm_add_ps(a0, a1);

		return StoreVector4f(a2);
	}
#endif

	namespace Math {

		// from: https://github.com/gszauer/GamePhysicsCookbook/blob/master/Code/matrices.cpp
		template <class T, size_t n, size_t m>
		TMatrix<T, m, n> QuickInverse(const TMatrix<T, m, n>& mat) {
			static_assert(m == n && m == 4, "QuickInverse only applies to 4x4 matrix");
			TType4<T> vv;
			vv.x() = SquareLength(TType4<T>(mat(0, 0), mat(0, 1), mat(0, 2), T(0)));
			vv.y() = SquareLength(TType4<T>(mat(1, 0), mat(1, 1), mat(1, 2), T(0)));
			vv.z() = SquareLength(TType4<T>(mat(2, 0), mat(2, 1), mat(2, 2), T(0)));
			vv.w() = 1;

			vv = TType4<T>(1, 1, 1, 1) / vv;
			vv.w() = 0;

			TMatrix<T, n, n> inverse = mat.Transpose();
			for (size_t i = 0; i < 3; i++) {
				inverse(i) = inverse(i) * vv;
			}

			TType4<T> right = TType4<T>(inverse(0, 0), inverse(1, 0), inverse(2, 0), T(0));
			TType4<T> up = TType4<T>(inverse(0, 1), inverse(1, 1), inverse(2, 1), T(0));
			TType4<T> forward = TType4<T>(inverse(0, 2), inverse(1, 2), inverse(2, 2), T(0));
			const TVector<T, 4>& position = mat(3);

			inverse(3, 0) = -DotProduct(right, position);
			inverse(3, 1) = -DotProduct(up, position);
			inverse(3, 2) = -DotProduct(forward, position);
			inverse(3, 3) = 1;

			return inverse;
		}

		template <class T>
		TMatrix<T, 3, 3> Inverse(const TMatrix<T, 3, 3>& m) {
			T det = m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) -
				m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
				m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));

			T invdet = T(1) / det;

			TMatrix<T, 3, 3> minv;
			minv(0, 0) = (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) * invdet;
			minv(0, 1) = (m(0, 2) * m(2, 1) - m(0, 1) * m(2, 2)) * invdet;
			minv(0, 2) = (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * invdet;
			minv(1, 0) = (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) * invdet;
			minv(1, 1) = (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * invdet;
			minv(1, 2) = (m(1, 0) * m(0, 2) - m(0, 0) * m(1, 2)) * invdet;
			minv(2, 0) = (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) * invdet;
			minv(2, 1) = (m(2, 0) * m(0, 1) - m(0, 0) * m(2, 1)) * invdet;
			minv(2, 2) = (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) * invdet;

			return minv;
		}

		template <class T>
		TMatrix<T, 4, 4> Inverse(const TMatrix<T, 4, 4>& m) {
			T det
				= m(0, 0) * m(1, 1) * m(2, 2) * m(3, 3) + m(0, 0) * m(1, 2) * m(2, 3) * m(3, 1) + m(0, 0) * m(1, 3) * m(2, 1) * m(3, 2)
				+ m(0, 1) * m(1, 0) * m(2, 3) * m(3, 2) + m(0, 1) * m(1, 2) * m(2, 0) * m(3, 3) + m(0, 1) * m(1, 3) * m(2, 2) * m(3, 0)
				+ m(0, 2) * m(1, 0) * m(2, 1) * m(3, 3) + m(0, 2) * m(1, 1) * m(2, 3) * m(3, 0) + m(0, 2) * m(1, 3) * m(2, 0) * m(3, 1)
				+ m(0, 3) * m(1, 0) * m(2, 2) * m(3, 1) + m(0, 3) * m(1, 1) * m(2, 0) * m(3, 2) + m(0, 3) * m(1, 2) * m(2, 1) * m(3, 0)
				- m(0, 0) * m(1, 1) * m(2, 3) * m(3, 2) - m(0, 0) * m(1, 2) * m(2, 1) * m(3, 3) - m(0, 0) * m(1, 3) * m(2, 2) * m(3, 1)
				- m(0, 1) * m(1, 0) * m(2, 2) * m(3, 3) - m(0, 1) * m(1, 2) * m(2, 3) * m(3, 0) - m(0, 1) * m(1, 3) * m(2, 0) * m(3, 2)
				- m(0, 2) * m(1, 0) * m(2, 3) * m(3, 1) - m(0, 2) * m(1, 1) * m(2, 0) * m(3, 3) - m(0, 2) * m(1, 3) * m(2, 1) * m(3, 0)
				- m(0, 3) * m(1, 0) * m(2, 1) * m(3, 2) - m(0, 3) * m(1, 1) * m(2, 2) * m(3, 0) - m(0, 3) * m(1, 2) * m(2, 0) * m(3, 1);

			if (fabs(det) < 0.000001f) {
				return TMatrix<T, 4, 4>::Identity();
			}

			T idet = T(1) / det;
			TMatrix<T, 4, 4> result;
			result(0, 0) = (m(1, 1) * m(2, 2) * m(3, 3) + m(1, 2) * m(2, 3) * m(3, 1) + m(1, 3) * m(2, 1) * m(3, 2) - m(1, 1) * m(2, 3) * m(3, 2) - m(1, 2) * m(2, 1) * m(3, 3) - m(1, 3) * m(2, 2) * m(3, 1)) * idet;
			result(0, 1) = (m(0, 1) * m(2, 3) * m(3, 2) + m(0, 2) * m(2, 1) * m(3, 3) + m(0, 3) * m(2, 2) * m(3, 1) - m(0, 1) * m(2, 2) * m(3, 3) - m(0, 2) * m(2, 3) * m(3, 1) - m(0, 3) * m(2, 1) * m(3, 2)) * idet;
			result(0, 2) = (m(0, 1) * m(1, 2) * m(3, 3) + m(0, 2) * m(1, 3) * m(3, 1) + m(0, 3) * m(1, 1) * m(3, 2) - m(0, 1) * m(1, 3) * m(3, 2) - m(0, 2) * m(1, 1) * m(3, 3) - m(0, 3) * m(1, 2) * m(3, 1)) * idet;
			result(0, 3) = (m(0, 1) * m(1, 3) * m(2, 2) + m(0, 2) * m(1, 1) * m(2, 3) + m(0, 3) * m(1, 2) * m(2, 1) - m(0, 1) * m(1, 2) * m(2, 3) - m(0, 2) * m(1, 3) * m(2, 1) - m(0, 3) * m(1, 1) * m(2, 2)) * idet;
			result(1, 0) = (m(1, 0) * m(2, 3) * m(3, 2) + m(1, 2) * m(2, 0) * m(3, 3) + m(1, 3) * m(2, 2) * m(3, 0) - m(1, 0) * m(2, 2) * m(3, 3) - m(1, 2) * m(2, 3) * m(3, 0) - m(1, 3) * m(2, 0) * m(3, 2)) * idet;
			result(1, 1) = (m(0, 0) * m(2, 2) * m(3, 3) + m(0, 2) * m(2, 3) * m(3, 0) + m(0, 3) * m(2, 0) * m(3, 2) - m(0, 0) * m(2, 3) * m(3, 2) - m(0, 2) * m(2, 0) * m(3, 3) - m(0, 3) * m(2, 2) * m(3, 0)) * idet;
			result(1, 2) = (m(0, 0) * m(1, 3) * m(3, 2) + m(0, 2) * m(1, 0) * m(3, 3) + m(0, 3) * m(1, 2) * m(3, 0) - m(0, 0) * m(1, 2) * m(3, 3) - m(0, 2) * m(1, 3) * m(3, 0) - m(0, 3) * m(1, 0) * m(3, 2)) * idet;
			result(1, 3) = (m(0, 0) * m(1, 2) * m(2, 3) + m(0, 2) * m(1, 3) * m(2, 0) + m(0, 3) * m(1, 0) * m(2, 2) - m(0, 0) * m(1, 3) * m(2, 2) - m(0, 2) * m(1, 0) * m(2, 3) - m(0, 3) * m(1, 2) * m(2, 0)) * idet;
			result(2, 0) = (m(1, 0) * m(2, 1) * m(3, 3) + m(1, 1) * m(2, 3) * m(3, 0) + m(1, 3) * m(2, 0) * m(3, 1) - m(1, 0) * m(2, 3) * m(3, 1) - m(1, 1) * m(2, 0) * m(3, 3) - m(1, 3) * m(2, 1) * m(3, 0)) * idet;
			result(2, 1) = (m(0, 0) * m(2, 3) * m(3, 1) + m(0, 1) * m(2, 0) * m(3, 3) + m(0, 3) * m(2, 1) * m(3, 0) - m(0, 0) * m(2, 1) * m(3, 3) - m(0, 1) * m(2, 3) * m(3, 0) - m(0, 3) * m(2, 0) * m(3, 1)) * idet;
			result(2, 2) = (m(0, 0) * m(1, 1) * m(3, 3) + m(0, 1) * m(1, 3) * m(3, 0) + m(0, 3) * m(1, 0) * m(3, 1) - m(0, 0) * m(1, 3) * m(3, 1) - m(0, 1) * m(1, 0) * m(3, 3) - m(0, 3) * m(1, 1) * m(3, 0)) * idet;
			result(2, 3) = (m(0, 0) * m(1, 3) * m(2, 1) + m(0, 1) * m(1, 0) * m(2, 3) + m(0, 3) * m(1, 1) * m(2, 0) - m(0, 0) * m(1, 1) * m(2, 3) - m(0, 1) * m(1, 3) * m(2, 0) - m(0, 3) * m(1, 0) * m(2, 1)) * idet;
			result(3, 0) = (m(1, 0) * m(2, 2) * m(3, 1) + m(1, 1) * m(2, 0) * m(3, 2) + m(1, 2) * m(2, 1) * m(3, 0) - m(1, 0) * m(2, 1) * m(3, 2) - m(1, 1) * m(2, 2) * m(3, 0) - m(1, 2) * m(2, 0) * m(3, 1)) * idet;
			result(3, 1) = (m(0, 0) * m(2, 1) * m(3, 2) + m(0, 1) * m(2, 2) * m(3, 0) + m(0, 2) * m(2, 0) * m(3, 1) - m(0, 0) * m(2, 2) * m(3, 1) - m(0, 1) * m(2, 0) * m(3, 2) - m(0, 2) * m(2, 1) * m(3, 0)) * idet;
			result(3, 2) = (m(0, 0) * m(1, 2) * m(3, 1) + m(0, 1) * m(1, 0) * m(3, 2) + m(0, 2) * m(1, 1) * m(3, 0) - m(0, 0) * m(1, 1) * m(3, 2) - m(0, 1) * m(1, 2) * m(3, 0) - m(0, 2) * m(1, 0) * m(3, 1)) * idet;
			result(3, 3) = (m(0, 0) * m(1, 1) * m(2, 2) + m(0, 1) * m(1, 2) * m(2, 0) + m(0, 2) * m(1, 0) * m(2, 1) - m(0, 0) * m(1, 2) * m(2, 1) - m(0, 1) * m(1, 0) * m(2, 2) - m(0, 2) * m(1, 1) * m(2, 0)) * idet;

			return result;
		}

#ifdef USE_SSE
		// From GLM
		template <>
		inline TMatrix<float, 4, 4> Inverse(const TMatrix<float, 4, 4>& m) {
			TMatrix<float, 4, 4> result;
			__m128 in[4];
			in[0] = LoadVector4f(m(0));
			in[1] = LoadVector4f(m(1));
			in[2] = LoadVector4f(m(2));
			in[3] = LoadVector4f(m(3));

			__m128 Fac0;
			{
				//	valType SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
				//	valType SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
				//	valType SubFactor06 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
				//	valType SubFactor13 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac0 = _mm_sub_ps(Mul00, Mul01);
			}

			__m128 Fac1;
			{
				//	valType SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
				//	valType SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
				//	valType SubFactor07 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
				//	valType SubFactor14 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac1 = _mm_sub_ps(Mul00, Mul01);
			}

			__m128 Fac2;
			{
				//	valType SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
				//	valType SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
				//	valType SubFactor08 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
				//	valType SubFactor15 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac2 = _mm_sub_ps(Mul00, Mul01);
			}

			__m128 Fac3;
			{
				//	valType SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
				//	valType SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
				//	valType SubFactor09 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
				//	valType SubFactor16 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac3 = _mm_sub_ps(Mul00, Mul01);
			}

			__m128 Fac4;
			{
				//	valType SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
				//	valType SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
				//	valType SubFactor10 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
				//	valType SubFactor17 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac4 = _mm_sub_ps(Mul00, Mul01);
			}

			__m128 Fac5;
			{
				//	valType SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
				//	valType SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
				//	valType SubFactor12 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
				//	valType SubFactor18 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

				__m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));
				__m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

				__m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
				__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
				__m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));

				__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
				__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
				Fac5 = _mm_sub_ps(Mul00, Mul01);
			}

			__m128 SignA = _mm_set_ps(1.0f, -1.0f, 1.0f, -1.0f);
			__m128 SignB = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);

			// m[1][0]
			// m[0][0]
			// m[0][0]
			// m[0][0]
			__m128 Temp0 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Vec0 = _mm_shuffle_ps(Temp0, Temp0, _MM_SHUFFLE(2, 2, 2, 0));

			// m[1][1]
			// m[0][1]
			// m[0][1]
			// m[0][1]
			__m128 Temp1 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Vec1 = _mm_shuffle_ps(Temp1, Temp1, _MM_SHUFFLE(2, 2, 2, 0));

			// m[1][2]
			// m[0][2]
			// m[0][2]
			// m[0][2]
			__m128 Temp2 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Vec2 = _mm_shuffle_ps(Temp2, Temp2, _MM_SHUFFLE(2, 2, 2, 0));

			// m[1][3]
			// m[0][3]
			// m[0][3]
			// m[0][3]
			__m128 Temp3 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Vec3 = _mm_shuffle_ps(Temp3, Temp3, _MM_SHUFFLE(2, 2, 2, 0));

			// col0
			// + (Vec1[0] * Fac0[0] - Vec2[0] * Fac1[0] + Vec3[0] * Fac2[0]),
			// - (Vec1[1] * Fac0[1] - Vec2[1] * Fac1[1] + Vec3[1] * Fac2[1]),
			// + (Vec1[2] * Fac0[2] - Vec2[2] * Fac1[2] + Vec3[2] * Fac2[2]),
			// - (Vec1[3] * Fac0[3] - Vec2[3] * Fac1[3] + Vec3[3] * Fac2[3]),
			__m128 Mul00 = _mm_mul_ps(Vec1, Fac0);
			__m128 Mul01 = _mm_mul_ps(Vec2, Fac1);
			__m128 Mul02 = _mm_mul_ps(Vec3, Fac2);
			__m128 Sub00 = _mm_sub_ps(Mul00, Mul01);
			__m128 Add00 = _mm_add_ps(Sub00, Mul02);
			__m128 Inv0 = _mm_mul_ps(SignB, Add00);

			// col1
			// - (Vec0[0] * Fac0[0] - Vec2[0] * Fac3[0] + Vec3[0] * Fac4[0]),
			// + (Vec0[0] * Fac0[1] - Vec2[1] * Fac3[1] + Vec3[1] * Fac4[1]),
			// - (Vec0[0] * Fac0[2] - Vec2[2] * Fac3[2] + Vec3[2] * Fac4[2]),
			// + (Vec0[0] * Fac0[3] - Vec2[3] * Fac3[3] + Vec3[3] * Fac4[3]),
			__m128 Mul03 = _mm_mul_ps(Vec0, Fac0);
			__m128 Mul04 = _mm_mul_ps(Vec2, Fac3);
			__m128 Mul05 = _mm_mul_ps(Vec3, Fac4);
			__m128 Sub01 = _mm_sub_ps(Mul03, Mul04);
			__m128 Add01 = _mm_add_ps(Sub01, Mul05);
			__m128 Inv1 = _mm_mul_ps(SignA, Add01);

			// col2
			// + (Vec0[0] * Fac1[0] - Vec1[0] * Fac3[0] + Vec3[0] * Fac5[0]),
			// - (Vec0[0] * Fac1[1] - Vec1[1] * Fac3[1] + Vec3[1] * Fac5[1]),
			// + (Vec0[0] * Fac1[2] - Vec1[2] * Fac3[2] + Vec3[2] * Fac5[2]),
			// - (Vec0[0] * Fac1[3] - Vec1[3] * Fac3[3] + Vec3[3] * Fac5[3]),
			__m128 Mul06 = _mm_mul_ps(Vec0, Fac1);
			__m128 Mul07 = _mm_mul_ps(Vec1, Fac3);
			__m128 Mul08 = _mm_mul_ps(Vec3, Fac5);
			__m128 Sub02 = _mm_sub_ps(Mul06, Mul07);
			__m128 Add02 = _mm_add_ps(Sub02, Mul08);
			__m128 Inv2 = _mm_mul_ps(SignB, Add02);

			// col3
			// - (Vec1[0] * Fac2[0] - Vec1[0] * Fac4[0] + Vec2[0] * Fac5[0]),
			// + (Vec1[0] * Fac2[1] - Vec1[1] * Fac4[1] + Vec2[1] * Fac5[1]),
			// - (Vec1[0] * Fac2[2] - Vec1[2] * Fac4[2] + Vec2[2] * Fac5[2]),
			// + (Vec1[0] * Fac2[3] - Vec1[3] * Fac4[3] + Vec2[3] * Fac5[3]));
			__m128 Mul09 = _mm_mul_ps(Vec0, Fac2);
			__m128 Mul10 = _mm_mul_ps(Vec1, Fac4);
			__m128 Mul11 = _mm_mul_ps(Vec2, Fac5);
			__m128 Sub03 = _mm_sub_ps(Mul09, Mul10);
			__m128 Add03 = _mm_add_ps(Sub03, Mul11);
			__m128 Inv3 = _mm_mul_ps(SignA, Add03);

			__m128 Row0 = _mm_shuffle_ps(Inv0, Inv1, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Row1 = _mm_shuffle_ps(Inv2, Inv3, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Row2 = _mm_shuffle_ps(Row0, Row1, _MM_SHUFFLE(2, 0, 2, 0));

			//	valType Determinant = m[0][0] * Inverse[0][0]
			//						+ m[0][1] * Inverse[1][0]
			//						+ m[0][2] * Inverse[2][0]
			//						+ m[0][3] * Inverse[3][0];
			__m128 Det0 = _Dot(in[0], Row2);
			__m128 Rcp0 = _mm_div_ps(_mm_set1_ps(1.0f), Det0);
			//__m128 Rcp0 = _mm_rcp_ps(Det0);

			//	Inverse /= Determinant;
			result(0) = StoreVector4f(_mm_mul_ps(Inv0, Rcp0));
			result(1) = StoreVector4f(_mm_mul_ps(Inv1, Rcp0));
			result(2) = StoreVector4f(_mm_mul_ps(Inv2, Rcp0));
			result(3) = StoreVector4f(_mm_mul_ps(Inv3, Rcp0));
			return result;
		}
#endif
		template <class T, size_t n>
		TMatrix<T, n, n> MatrixScale(const TVector<T, n>& v) {
			TMatrix<T, n, n> mat = TMatrix<T, n, n>::Identity();
			for (size_t i = 0; i < n; i++) {
				mat.data[i][i] = v[i];
			}

			return mat;
		}

		template <class T>
		TMatrix<T, 4, 4> MatrixTranslate(const TVector<T, 3>& v) {
			TMatrix<T, 4, 4> ret;
			ret.data[0][0] = 1;
			ret.data[0][1] = ret.data[0][2] = ret.data[0][3] = 0;

			ret.data[1][1] = 1;
			ret.data[1][0] = ret.data[1][2] = ret.data[1][3] = 0;

			ret.data[2][2] = 1;
			ret.data[2][0] = ret.data[2][1] = ret.data[2][3] = 0;

			ret.data[3][0] = v[0];
			ret.data[3][1] = v[1];
			ret.data[3][2] = v[2];
			ret.data[3][3] = 1;

			return ret;
		}

		template <class T>
		TMatrix<T, 4, 4> MatrixRotate(const TVector<T, 4>& dir) {
			T c = (T)cos(dir[3]);
			T s = (T)sin(dir[3]);

			TMatrix<T, 4, 4> mat;
			mat.data[0][0] = c + (1 - c) * dir[0] * dir[0];
			mat.data[0][1] = (1 - c) * dir[1] * dir[0] + s * dir[2];
			mat.data[0][2] = (1 - c) * dir[2] * dir[0] - s * dir[1];
			mat.data[0][3] = 0;
			mat.data[1][0] = (1 - c) * dir[0] * dir[1] - s * dir[2];
			mat.data[1][1] = c + (1 - c) * dir[1] * dir[1];
			mat.data[1][2] = (1 - c) * dir[2] * dir[1] + s * dir[0];
			mat.data[1][3] = 0;
			mat.data[2][0] = (1 - c) * dir[0] * dir[2] + s * dir[1];
			mat.data[2][1] = (1 - c) * dir[1] * dir[2] - s * dir[0];
			mat.data[2][2] = c + (1 - c) * dir[2] * dir[2];
			mat.data[2][3] = 0;
			mat.data[3][0] = mat.data[3][1] = mat.data[3][2] = 0;
			mat.data[3][3] = 1;

			return mat;
		}

		template <class T>
		TMatrix<T, 4, 4> MatrixOrtho(const TVector<T, 3>& size) {
			TMatrix<T, 4, 4> ret;
			ret.data[0][0] = (T)1 / size[0];
			ret.data[0][1] = ret.data[0][2] = ret.data[0][3] = 0;

			ret.data[1][1] = (T)1 / size[1];
			ret.data[1][0] = ret.data[1][2] = ret.data[1][3] = 0;

			ret.data[2][2] = (T)2 / size[2];
			ret.data[2][0] = ret.data[2][1] = ret.data[2][3] = 0;

			ret.data[3][3] = 1;
			ret.data[3][0] = ret.data[3][1] = ret.data[3][2] = 0;

			return ret;
		}

		template <class T>
		TMatrix<T, 4, 4> MatrixPerspective(T d, T rr, T n, T f) {
			T t = n * (T)tan(d / 2);
			T r = rr * t;
			TMatrix<T, 4, 4> ret;
			ret.data[0][0] = n / r;
			ret.data[0][1] = ret.data[0][2] = ret.data[0][3] = 0;

			ret.data[1][1] = n / t;
			ret.data[1][0] = ret.data[1][2] = ret.data[1][3] = 0;

			ret.data[2][2] = (f + n) / (f - n);
			ret.data[2][3] = -1;
			ret.data[2][0] = ret.data[2][1] = 0;

			ret.data[3][2] = 2 * f * n / (f - n);
			ret.data[3][0] = ret.data[3][1] = ret.data[3][3] = 0;

			return ret;
		}

		template <class T>
		TType4<T> CompressPerspective(const TMatrix<T, 4, 4>& matrix) {
			return TType4<T>(matrix.data[0][0], matrix.data[1][1], matrix.data[2][2], matrix.data[3][2]);
		}

		template <class T>
		TType4<T> CompressInversePerspective(const TMatrix<T, 4, 4>& matrix) {
			return TType4<T>(1.0f / matrix.data[0][0], 1.0f / matrix.data[1][1], matrix.data[2][2], matrix.data[3][2]);
		}

		template <class T>
		TMatrix<T, 4, 4> InversePerspective(const TMatrix<T, 4, 4>& m) {
			T a = m(0, 0);
			T b = m(1, 1);
			T c = m(2, 2);
			T d = m(3, 2);
			T s = m(2, 0);
			T t = m(2, 1);

			TMatrix<T, 4, 4> ret;
			ret.data[0][0] = 1 / a;
			ret.data[0][1] = ret.data[0][2] = ret.data[0][3] = 0;

			ret.data[1][1] = 1 / b;
			ret.data[1][0] = ret.data[1][2] = ret.data[1][3] = 0;

			ret.data[2][3] = 1 / d;
			ret.data[2][0] = ret.data[2][1] = ret.data[2][2] = 0;

			ret.data[3][0] = -s / a;
			ret.data[3][1] = -t / b;
			ret.data[3][2] = -1;
			ret.data[3][3] = c / d;

			return ret;
		}

		template <class T>
		TMatrix<T, 4, 4> MatrixLookAt(const TVector<T, 3>& position, const TVector<T, 3>& dir, const TVector<T, 3>& u) {
			TVector<T, 3> direction = dir;
			TVector<T, 3> up = u;

			direction = Normalize(direction);
			up = Normalize(up);

			/* Side = forward x up */
			TVector<T, 3> side = CrossProduct(direction, up);
			side = Normalize(side);
			up = CrossProduct(side, direction);

			TMatrix<T, 4, 4> m;

			m(0, 0) = side[0];
			m(1, 0) = side[1];
			m(2, 0) = side[2];
			m(3, 0) = 0;

			m(0, 1) = up[0];
			m(1, 1) = up[1];
			m(2, 1) = up[2];
			m(3, 1) = 0;

			m(0, 2) = -direction[0];
			m(1, 2) = -direction[1];
			m(2, 2) = -direction[2];
			m(3, 2) = 0;

			m(0, 3) = 0;
			m(1, 3) = 0;
			m(2, 3) = 0;
			m(3, 3) = 1;

			return MatrixTranslate(-position) * m;
		}

		template <class T>
		TVector<T, 3> Transform(const TMatrix<T, 4, 4>& input, const TVector<T, 3>& v) {
			TVector<T, 4> position;
			position[0] = v[0];
			position[1] = v[1];
			position[2] = v[2];
			position[3] = 1;

			position = position * input;

			TVector<T, 3> r;
			r[0] = position[0] / position[3];
			r[1] = position[1] / position[3];
			r[2] = position[2] / position[3];

			return r;
		}

		template <class T>
		void IntersectTriangle(TVector<T, 3>& res, TVector<T, 2>& uv, const TVector<T, 3> face[3], const std::pair<TVector<T, 3>, TVector<T, 3> >& vec) {
			// handle size!!!!
			const TVector<T, 3>& v = vec.second;
			const TVector<T, 3>& u = vec.first;
			const TVector<T, 3>& base = face[0];
			const TVector<T, 3>& m = face[1];
			const TVector<T, 3>& n = face[2];

			TVector<T, 3> N = n - base;
			TVector<T, 3> M = m - base;

			// make linear equations
			TMatrix<T, 3, 3> mat;
			mat(0) = M;
			mat(1) = N;
			mat(2) = -v;
			mat = Inverse(mat);

			TVector<T, 3> target = (u - base) * mat;

			T alpha = target[0];
			T beta = target[1];

			res = base + M * alpha + N * beta;

			uv[0] = alpha;
			uv[1] = beta;
		}

		template <class T>
		std::pair<TType3<T>, TType3<T> > QuickRay(const std::pair<TType3<T>, TType3<T> >& ray) {
			TType3<T> invDir;
			for (size_t i = 0; i < 3; i++) {
				invDir[i] = 1.0f / (fabs(ray.second[i]) > 0.000001f ? ray.second[i] : 0.000001f * (ray.second[i] > 0 ? 1 : -1));
			}

			return std::make_pair(ray.first * invDir, invDir);
		}

		// https://github.com/gszauer/GamePhysicsCookbook/blob/master/Code/Geometry3D.cpp
		template <class T>
		T IntersectBox(const std::pair<TType3<T>, TType3<T> >& aabb, const std::pair<TType3<T>, TType3<T> >& quickRay) {
			const TVector<T, 3>& minValue = aabb.first;
			const TVector<T, 3>& maxValue = aabb.second;

			TVector<T, 3> t135 = minValue * quickRay.second - quickRay.first;
			TVector<T, 3> t246 = maxValue * quickRay.second - quickRay.first;

			T tmax = (T)Min(Min(Max(t135[0], t246[0]), Max(t135[1], t246[1])), Max(t135[2], t246[2]));

			if (tmax < 0) {
				return tmax;
			} else {
				T tmin = (T)Max(Max(Min(t135[0], t246[0]), Min(t135[1], t246[1])), Min(t135[2], t246[2]));

				if (tmin > tmax) {
					return -tmin;
				} else {
					return Max(tmin, (T)0);
				}
			}
		}

		/*
#ifdef USE_SSE
		template <>
		float IntersectBox<float>(const std::pair<TType3<float>, TType3<float>>& aabb, const std::pair<TType3<float>, TType3<float>>& quickRay) {
			__m128 minValue = LoadVector4f(TType4<float>::Load(aabb.first));
			__m128 maxValue = LoadVector4f(TType4<float>::Load(aabb.second));
			__m128 quickRayFirst = LoadVector4f(TType4<float>::Load(quickRay.first));
			__m128 quickRaySecond = LoadVector4f(TType4<float>::Load(quickRay.second));

			__m128 t135 = _mm_sub_ps(_mm_mul_ps(minValue, quickRaySecond), quickRayFirst);
			__m128 t246 = _mm_sub_ps(_mm_mul_ps(maxValue, quickRaySecond), quickRayFirst);

			TType4<float> smin = StoreVector4f(_mm_min_ps(t135, t246));
			TType4<float> smax = StoreVector4f(_mm_max_ps(t135, t246));

			float tmin = Max(Max(smin.x(), smin.y()), smin.z());
			float tmax = Min(Min(smax.x(), smax.y()), smax.z());

			if (tmax < 0) {
				return tmax;
			} else if (tmin > tmax) {
				return -tmin;
			} else {
				return Max(tmin, 0.0f);
			}
		}
#endif
		*/
	}

	template <class T>
	class TQuaternion : public TType4<T> {
	public:
		TQuaternion(T ww = 1, T xx = 0, T yy = 0, T zz = 0) : TType4<T>(xx, yy, zz, ww) {}
		TQuaternion(const TType4<T>& quat) : TType4<T>(quat) {}
		TQuaternion(const TType3<T>& rot) {
			const T fSinPitch((T)sin(rot.y() * 0.5));
			const T fCosPitch((T)cos(rot.y() * 0.5));
			const T fSinYaw((T)sin(rot.z() * 0.5));
			const T fCosYaw((T)cos(rot.z() * 0.5));
			const T fSinRoll((T)sin(rot.x() * 0.5));
			const T fCosRoll((T)cos(rot.x() * 0.5));
			const T fCosPitchCosYaw(fCosPitch * fCosYaw);
			const T fSinPitchSinYaw(fSinPitch * fSinYaw);

			this->x() = fSinRoll * fCosPitchCosYaw - fCosRoll * fSinPitchSinYaw;
			this->y() = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
			this->z() = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
			this->w() = fCosRoll * fCosPitchCosYaw + fSinRoll * fSinPitchSinYaw;

			Normalize();
		}

		static T Distance(const TQuaternion& lhs, const TQuaternion& rhs) {
			TQuaternion conj = rhs;
			conj.Conjugate();
			return (lhs * conj).w();
		}

		TQuaternion(const TMatrix<T, 4, 4>& m) { // from OGRE
			T tq[4];
			int i, j;

			// Use tq to store the largest trace
			tq[0] = 1 + m(0, 0) + m(1, 1) + m(2, 2);
			tq[1] = 1 + m(0, 0) - m(1, 1) - m(2, 2);
			tq[2] = 1 - m(0, 0) + m(1, 1) - m(2, 2);
			tq[3] = 1 - m(0, 0) - m(1, 1) + m(2, 2);

			// Find the maximum (could also use stacked if's later)
			j = 0;
			for (i = 1; i < 4; i++) j = (tq[i] > tq[j]) ? i : j;

			// check the diagonal
			if (j == 0) {
				/* perform instant calculation */
				this->w() = tq[0];
				this->x() = m(2, 1) - m(1, 2);
				this->y() = m(0, 2) - m(2, 0);
				this->z() = m(1, 0) - m(0, 1);
			} else if (j == 1) {
				this->w() = m(2, 1) - m(1, 2);
				this->x() = tq[1];
				this->y() = m(1, 0) + m(0, 1);
				this->z() = m(0, 2) + m(2, 0);
			} else if (j == 2) {
				this->w() = m(0, 2) - m(2, 0);
				this->x() = m(1, 0) + m(0, 1);
				this->y() = tq[2];
				this->z() = m(2, 1) + m(1, 2);
			} else {
				this->w() = m(1, 0) - m(0, 1);
				this->x() = m(0, 2) + m(2, 0);
				this->y() = m(2, 1) + m(1, 2);
				this->z() = tq[3];
			}

			Normalize();
		}

		static TQuaternion Flip() {
			return TQuaternion(0, 0, 0, 0);
		}

		static TQuaternion Align(const TType3<T>& from, const TType3<T>& to) {
			const T EPSILON = (T)1e-3;
			TType3<T> axis = CrossProduct(to, from);
			T pcos = DotProduct(from, to);
			T halfcos = (T)sqrt(0.5 + pcos * 0.5);
			T ratio = halfcos > EPSILON ? (T)(0.5 / halfcos) : 0;

			return TQuaternion(halfcos, axis.x() * ratio, axis.y() * ratio, axis.z() * ratio);
		}

		TType3<T> ToEulerAngle() const {
			T xx = (T)atan2(2 * (this->w() * this->x() + this->y() * this->z()), 1 - 2.0 * (this->x() * this->x() + this->y() * this->y()));
			T yy = (T)asin(2 * (this->w() * this->y() - this->z() * this->x()));
			T zz = (T)atan2(2 * (this->w() * this->z() + this->x() * this->y()), 1 - 2.0 * (this->y() * this->y() + this->z() * this->z()));

			return TType3<T>(xx, yy, zz);
		}

		TQuaternion& Normalize() {
			T mag = sqrt(this->x() * this->x() + this->y() * this->y() + this->z() * this->z() + this->w() * this->w());
			if (mag > 1e-6) {
				mag = 1.0f / mag;
				this->x() *= mag;
				this->y() *= mag;
				this->z() *= mag;
				this->w() *= mag;
			}

			return *this;
		}

		bool IsFlip() const {
			return this->x() == 0 && this->y() == 0 && this->z() == 0 && this->w() == 0;
		}

		TQuaternion operator * (const TQuaternion& t) const {
			assert(!this->IsFlip() && !t.IsFlip());
			return TQuaternion(this->w() * t.w() - this->x() * t.x() - this->y() * t.y() - this->z() * t.z(),
				this->w() * t.x() + this->x() * t.w() + this->y() * t.z() - this->z() * t.y(),
				this->w() * t.y() + this->y() * t.w() + this->z() * t.x() - this->x() * t.z(),
				this->w() * t.z() + this->z() * t.w() + this->x() * t.y() - this->y() * t.x());
		}

		TQuaternion& operator *= (const TQuaternion& t) {
			*this = *this * t;
			return *this;
		}

		TQuaternion& Conjugate() {
			this->x() = -this->x();
			this->y() = -this->y();
			this->z() = -this->z();

			return *this;
		}

		void Transform(TType3<T>& v) const {
			v = (*this)(v);
		}

		TType3<T> operator () (const TType3<T>& v) const {
			if (IsFlip()) {
				return TType3<T>(-v.x(), -v.y(), -v.z());
			} else {
				TQuaternion q2(0, v.x(), v.y(), v.z()), q = *this, qinv = *this;
				q.Conjugate();

				q = q * q2 * qinv;
				return TType3<T>(q.x(), q.y(), q.z());
			}
		}

		static void Interpolate(TQuaternion& out, const TQuaternion& start, const TQuaternion& end, T factor) {
			assert(!start.IsFlip() && !end.IsFlip());
			T cosom = start.x() * end.x() + start.y() * end.y() + start.z() * end.z() + start.w() * end.w();

			TQuaternion qend = end;
			if (cosom < 0) {
				cosom = -cosom;
				qend.x() = -qend.x();
				qend.y() = -qend.y();
				qend.z() = -qend.z();
				qend.w() = -qend.w();
			}

			T sclp, sclq;
			if ((T)1.0 - cosom > 1e-6) {
				T omega, sinom;
				omega = acos(cosom);
				sinom = sin(omega);
				sclp = sin((1.0f - factor) * omega) / sinom;
				sclq = sin(factor * omega) / sinom;
			} else {
				sclp = (T)1.0 - factor;
				sclq = factor;
			}

			out.x() = sclp * start.x() + sclq * qend.x();
			out.y() = sclp * start.y() + sclq * qend.y();
			out.z() = sclp * start.z() + sclq * qend.z();
			out.w() = sclp * start.w() + sclq * qend.w();
		}

		static void InterpolateSquad(TQuaternion& out, const TQuaternion& left, const TQuaternion& outTan, const TQuaternion& right, const TQuaternion& inTan, T factor) {
			T t = (T)(2.0 * factor * (1.0 - factor));
			TQuaternion p, q;
			Interpolate(p, left, right, t);
			Interpolate(q, outTan, inTan, t);

			Interpolate(out, p, q, t);
		}

		void WriteMatrix(TMatrix<T, 4U, 4U>& m) const {
			if (!IsFlip()) {
				T* mat = &m.data[0][0];
				mat[0] = 1 - 2 * (this->y() * this->y() + this->z() * this->z());
				mat[1] = 2 * (this->x() * this->y() - this->z() * this->w());
				mat[2] = 2 * (this->x() * this->z() + this->y() * this->w());
				mat[3] = 0;
				mat[4] = 2 * (this->x() * this->y() + this->z() * this->w());
				mat[5] = 1 - 2 * (this->x() * this->x() + this->z() * this->z());
				mat[6] = 2 * (this->y() * this->z() - this->x() * this->w());
				mat[7] = 0;
				mat[8] = 2 * (this->x() * this->z() - this->y() * this->w());
				mat[9] = 2 * (this->y() * this->z() + this->x() * this->w());
				mat[10] = 1 - 2 * (this->x() * this->x() + this->y() * this->y());
				mat[11] = mat[12] = mat[13] = mat[14] = 0;
				mat[15] = 1;
			} else {
				T mat[16] = { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1 };
				m = TMatrix<T, 4U, 4U>(mat);
			}
		}
	};
}

