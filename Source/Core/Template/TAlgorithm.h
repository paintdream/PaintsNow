// TAlgorithm.h -- Basic algorithms
// PaintDream (paintdream@paintdream.com)
// 2014-12-2
//

#pragma once
#include "../PaintsNow.h"
#include <cstring>
#include <algorithm>
#include <functional>

namespace PaintsNow {
	namespace Math {
		template <class T>
		T Max(T a, T b) {
			return a > b ? a : b;
		}

		template <class T>
		T Min(T a, T b) {
			return a < b ? a : b;
		}

		template <class T>
		T Clamp(T x, T a, T b) {
			return x < a ? a : x > b ? b : x;
		}

		// Min for vectors.
		template <class T>
		T AllMin(const T& lhs, const T& rhs) {
			T result;
			for (size_t i = 0; i < T::size; i++) {
				result[i] = Math::Min(lhs[i], rhs[i]);
			}

			return result;
		}

		// Max for vectors
		template <class T>
		T AllMax(const T& lhs, const T& rhs) {
			T result;
			for (size_t i = 0; i < T::size; i++) {
				result[i] = Math::Max(lhs[i], rhs[i]);
			}

			return result;
		}

		// Clamp for vectors
		template <class T>
		T AllClamp(const T& v, const T& lhs, const T& rhs) {
			T result;
			for (size_t i = 0; i < T::size; i++) {
				result[i] = Clamp(v[i], lhs[i], rhs[i]);
			}

			return result;
		}

		// Interpolate for scalars and vectors
		template <class T, class D>
		T Interpolate(const T& a, const T& b, D alpha) {
			return a * (1 - alpha) + b * alpha;
		}

		// Quantize vector to integer with given range [0, divCount]
		template <class T, class D>
		D QuantizeVector(const std::pair<T, T>& bound, const T& value, const D& divCount) {
			typedef typename T::type floattype;
			typedef typename D::type inttype;

			D result;
			for (size_t i = 0; i < sizeof(T) / sizeof(floattype); i++) {
				result[i] = Math::Clamp((inttype)(divCount[i] * (value[i] - bound.first[i]) / (bound.second[i] - bound.first[i])), (inttype)0, divCount[i]);
			}

			return result;
		}
	
		// Get alignment of a number
		template <class T>
		T Alignment(T a) {
			return a & (~a + 1); // the same as a & -a, but no compiler warnings.
		}

		template <class T>
		T AlignmentTo(T value, T alignment) {
			return (value + (alignment - 1)) & ~(alignment - 1);
		}

		// Get alignement mask of a number
		template <class T>
		T AlignmentMask(T a) {
			return ~(Alignment(a) - 1);
		}

		template <class T>
		T LogAlignmentTop(T t) {
			T z = 0;
			while (t != 0) {
				t >>= 1;
				z++;
			}

			return z;
		}

		template <class T>
		T AlignmentTop(T a) {
			T z = 1, t = a;
			while ((t >>= 1) != 0) {
				z <<= 1;
			}

			return z;
		}

		template <class T>
		T AlignmentRange(T a, T b) {
			T top = AlignmentTop(a ^ b);
			return (a & ~(top - 1)) | top;
		}

		template <class T>
		T SiteCount(T a, T b) {
			T c = AlignmentRange(a, b);
			T count = 0;
			if (a != 0) {
				while (a < c) {
					a += Math::Alignment(a);
					count++;
				}
			} else {
				count++;
			}

			if (b != 0) {
				while (b > c) {
					b -= Math::Alignment(b);
					count++;
				}
			}

			return count;
		}

		template <class T>
		T ReverseBytes(const T& src) {
			T dst = src;
			char* ptr = reinterpret_cast<char*>(&dst);
			for (size_t i = 0; i < sizeof(T) / 2; i++) {
				std::swap(ptr[i], ptr[sizeof(T) - i - 1]);
			}

			return dst;
		}
	}

	// Binary find / insert / remove extension of std::vector<> like containers.
	template <class K, class V>
	class KeyValue : public std::pair<K, V> {
	public:
		typedef std::pair<K, V> base;
#if defined(_MSC_VER) && _MSC_VER <= 1200
		KeyValue(const K& k = K(), const V& v = V()) : std::pair<K, V>(k, v) {}
		KeyValue(rvalue<KeyValue> v) : std::pair<K, V>(std::move(v.first), std::move(v.second)) {}
#else
		template <class KK, class VV>
		KeyValue(KK&& k, VV&& v) : std::pair<K, V>(std::forward<KK>(k), std::forward<VV>(v)) {}
		KeyValue(const K& k) : std::pair<K, V>(k, V()) {}
		KeyValue() {}
#endif
		bool operator == (const KeyValue& rhs) const {
			return base::first == rhs.first;
		}

		bool operator < (const KeyValue& rhs) const {
			return base::first < rhs.first;
		}
	};

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <class K, class V>
	KeyValue<std::decay<K>::type, std::decay<V>::type> MakeKeyValue(const K& k, const V& v) {
		return KeyValue<std::decay<K>::type, std::decay<V>::type>(k, v);
	}
#else
	template <class K, class V>
	KeyValue<typename std::decay<K>::type, typename std::decay<V>::type> MakeKeyValue(K&& k, V&& v) {
		return KeyValue<typename std::decay<K>::type, typename std::decay<V>::type>(std::forward<K>(k), std::forward<V>(v));
	}
#endif
	template <class T, class D>
	T CastKeyValue(const D& d, const T*) {
		return d;
	}

	template <class I, class D, class P>
	I BinaryFind(I begin, I end, const D& value, const P& pred) {
		if (begin == end) return end;
		I it = lower_bound(begin, end, CastKeyValue(value, &*begin), pred);
		return it != end && !pred(CastKeyValue(value, &*begin), *it) ? it : end;
	}

	template <class I, class D>
	I BinaryFind(I begin, I end, const D& value) {
		if (begin == end) return end;
		I it = lower_bound(begin, end, CastKeyValue(value, &*begin));
		return it != end && !(CastKeyValue(value, &*begin) < *it) ? it : end;
	}

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <class T, class D, class P>
	typename T::iterator BinaryInsert(T& container, rvalue<D> value, const P& pred) {
		typename T::iterator it = upper_bound(container.begin(), container.end(), (typename T::value_type)value, pred);
		typename T::iterator ip = it;
		if (it != container.begin() && !pred(*--ip, (typename T::value_type)value)) {
			*ip = std::move(value);
			return ip;
		} else {
			return container.insert(it, (typename T::value_type)value);
		}
	}

	template <class T, class D, class P>
	typename T::iterator BinaryInsert(T& container, const D& value, const P& pred) {
		typename T::iterator it = upper_bound(container.begin(), container.end(), (typename T::value_type)value, pred);
		typename T::iterator ip = it;
		if (it != container.begin() && !pred(*--ip, (typename T::value_type)value)) {
			*ip = value;
			return ip;
		} else {
			return container.insert(it, (typename T::value_type)value);
		}
	}

	template <class T, class D>
	typename T::iterator BinaryInsert(T& container, const D& value) {
		typename T::iterator it = upper_bound(container.begin(), container.end(), (typename T::value_type)value);
		typename T::iterator ip = it;
		if (it != container.begin() && !(*--ip < (typename T::value_type)value)) {
			*ip = value;
			return ip;
		} else {
			return container.insert(it, (typename T::value_type)value);
		}
	}
#else
	template <class T, class D, class P>
	typename T::iterator BinaryInsert(T& container, D&& value, const P& pred) {
		typename T::iterator it = upper_bound(container.begin(), container.end(), (typename T::value_type)value, pred);
		typename T::iterator ip = it;
		if (it != container.begin() && !pred(*--ip, (typename T::value_type)value)) {
			*ip = std::forward<D>(value);
			return ip;
		} else {
			return container.insert(it, std::forward<D>(value));
		}
	}

	template <class T, class D>
	typename T::iterator BinaryInsert(T& container, D&& value) {
		typename T::iterator it = upper_bound(container.begin(), container.end(), (typename T::value_type)value);
		typename T::iterator ip = it;
		if (it != container.begin() && !(*--ip < (typename T::value_type)value)) {
			*ip = std::forward<D>(value);
			return ip;
		} else {
			return container.insert(it, std::forward<D>(value));
		}
	}
#endif
	template <class T, class D, class P>
	bool BinaryErase(T& container, const D& value, const P& pred) {
		typename T::iterator it = upper_bound(container.begin(), container.end(), (typename T::value_type)value, pred);
		typename T::iterator ip = it;
		if (it != container.begin() && !pred(*--ip, (typename T::value_type)value)) {
			container.erase(ip);
			return true;
		} else {
			return false;
		}
	}

	template <class T, class D>
	bool BinaryErase(T& container, const D& value) {
		typename T::iterator it = upper_bound(container.begin(), container.end(), (typename T::value_type)value);
		typename T::iterator ip = it;
		if (it != container.begin() && !(*--ip < (typename T::value_type)value)) {
			container.erase(ip);
			return true;
		} else {
			return false;
		}
	}
}

