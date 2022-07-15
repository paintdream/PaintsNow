// ZType.h -- Basic type instances
// PaintDream (paintdream@paintdream.com)
// 2014-11-30
//

#pragma once
#ifdef _MSC_VER
#pragma warning (disable:4786)
#pragma warning (disable:4503)
#endif // _MSC_VER

#include "../PaintsNow.h"
#include "../Template/TVector.h"
#include "../Template/TMatrix.h"
#include <cmath>
#include <string>
#include <vector>
#include <stdexcept>
#include <cfloat>

#ifndef __STD_TYPES__
#define __STD_TYPES__
#ifdef _MSC_VER
#if (_MSC_VER <= 1200)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef __int64 int64_t;

#include "../Backport/VC98STRING.h"
#else
#include <stdint.h>
#endif
#else
#include <stdint.h>
// typedef unsigned long long uint64_t;
#endif
#endif

namespace PaintsNow {
#if defined(_MSC_VER) && _MSC_VER <= 1200
	typedef std::string_mt String;
	std::string Utf8ToStd(const String& str);
	String StdToUtf8(const std::string& str);
#else
	typedef std::string String;
#define StdToUtf8(f) (f)
#define Utf8ToStd(f) (f)
#endif
	inline /*constexpr*/ uint64_t HashBuffer(const void* buffer, size_t length, uint64_t seed = 0xEEEEEEEE7FED7FED) {
		unsigned char *key = (unsigned char *)buffer;
		unsigned char *end = key + length;
		uint32_t seed1 = (uint32_t)seed;
		uint32_t seed2 = (uint32_t)((seed >> 31) >> 1);

		int ch;
		while (key != end) {
			ch = *key++;
			seed1 = ch ^ (seed1 + seed2);
			seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
		}

		return (uint64_t)seed1 | (((uint64_t)seed2 << 31) << 1);
	}

	template <class T>
	inline /*constexpr*/ uint64_t HashValue(const T& value, uint64_t seed = 0xEEEEEEEE7FED7FED) {
		return HashBuffer(&value, sizeof(value), seed);
	}

	std::vector<String> Split(const String& str, char sep = ' ');

	class StringView {
	private:
		StringView(rvalue<String> str); // not allowed

	public:
		StringView() : buffer(nullptr), viewSize(0) {}
		StringView(const String& str) : buffer(str.data()), viewSize(str.size()) {}
		StringView(const char* str, size_t len) : buffer(str), viewSize(len) {}
		StringView(const char* str) : buffer(str), viewSize(strlen(str)) {}

		operator String() const {
			return String(buffer, viewSize);
		}

		size_t size() const { return viewSize; }
		size_t length() const { return viewSize; }
		const char* data() const { return buffer; }
		const char* begin() const { return buffer; }
		const char* end() const { return buffer + viewSize; }
		const char* find(const char ch) const { return std::find(buffer, buffer + viewSize, ch); }
		const char operator [] (size_t i) const { assert(i < viewSize); return buffer[i]; }

		bool operator == (const StringView& rhs) const {
			if (viewSize != rhs.viewSize) return false;
			else if (viewSize == 0) return true;

			return memcmp(buffer, rhs.buffer, viewSize) == 0;
		}

		bool operator < (const StringView& rhs) const {
			if (viewSize == rhs.viewSize) {
				return memcmp(buffer, rhs.buffer, viewSize) < 0;
			} else if (viewSize > rhs.viewSize) {
				if (rhs.viewSize == 0) return false;
				int r = memcmp(buffer, rhs.buffer, rhs.viewSize);
				return r != 0 ? r < 0 : false;
			} else {
				if (viewSize == 0) return true;
				int r = memcmp(buffer, rhs.buffer, viewSize);
				return r != 0 ? r < 0 : true;
			}
		}

		StringView substr(size_t pos = 0, size_t count = String::npos) const {
			assert(buffer != nullptr);
			return StringView(buffer + pos, count == String::npos ? viewSize - pos : count);
		}

	private:
		const char* buffer;
		size_t viewSize;
	};

	typedef TType2<char> Char2;
	typedef TType3<char> Char3;
	typedef TType4<char> Char4;
	typedef TType2<unsigned char> UChar2;
	typedef TType3<unsigned char> UChar3;
	typedef TType4<unsigned char> UChar4;
	typedef TType2<short> Short2;
	typedef TType3<short> Short3;
	typedef TType4<short> Short4;
	typedef TType2<unsigned short> UShort2;
	typedef TType3<unsigned short> UShort3;
	typedef TType4<unsigned short> UShort4;
	typedef TType2<int> Int2;
	typedef TType3<int> Int3;
	typedef TType4<int> Int4;
	typedef TType2<unsigned int> UInt2;
	typedef TType3<unsigned int> UInt3;
	typedef TType4<unsigned int> UInt4;
	typedef TType2<float> Float2;
	typedef TType3<float> Float3;
#if defined(_MSC_VER) && _MSC_VER <= 1200
	typedef __declspec(align(16)) TType4<float> Float4;
#else
	typedef TType4<float> Float4;
#endif
	typedef TType2<double> Double2;
	typedef TType3<double> Double3;
	typedef TType4<double> Double4;
	typedef std::pair<Float4, Float4> Float4Pair;
	typedef std::pair<Float3, Float3> Float3Pair;
	typedef std::pair<Float2, Float2> Float2Pair;
	typedef std::pair<Double4, Double4> Double4Pair;
	typedef std::pair<Double3, Double3> Double3Pair;
	typedef std::pair<Double2, Double2> Double2Pair;
	typedef std::pair<Int4, Int4> Int4Pair;
	typedef std::pair<Int3, Int3> Int3Pair;
	typedef std::pair<Int2, Int2> Int2Pair;
	typedef std::pair<UInt4, UInt4> UInt4Pair;
	typedef std::pair<UInt3, UInt3> UInt3Pair;
	typedef std::pair<UInt2, UInt2> UInt2Pair;
	typedef std::pair<Short4, Short4> Short4Pair;
	typedef std::pair<Short3, Short3> Short3Pair;
	typedef std::pair<Short2, Short2> Short2Pair;
	typedef std::pair<UShort4, UShort4> UShort4Pair;
	typedef std::pair<UShort3, UShort3> UShort3Pair;
	typedef std::pair<UShort2, UShort2> UShort2Pair;
#if defined(_MSC_VER) && _MSC_VER <= 1200
	typedef __declspec(align(16)) TMatrix<float, 4U, 4U> MatrixFloat4x4;
#else
	typedef TMatrix<float, 4U, 4U> MatrixFloat4x4;
#endif
	typedef TMatrix<float, 3U, 4U> MatrixFloat3x4;
	typedef TMatrix<float, 3U, 3U> MatrixFloat3x3;
	typedef TMatrix<int, 3U, 3U> MatrixInt3x3;
	typedef TQuaternion<float> QuaternionFloat;

#ifdef _WIN32
	String Utf8ToSystem(const String& str);
	String SystemToUtf8(const String& str);
#else
#define Utf8ToSystem(f) (f)
#define SystemToUtf8(f) (f)
#endif
	namespace Math {
		float HalfToFloat(uint16_t value);

		template <class R, class T>
		R Interleave(TypeTrait<R>, const T* data, uint32_t len) {
			uint32_t level = sizeof(R) * 8 / len;
			T bitMask = verify_cast<T>(1 << level);
			R code = 0;

			for (uint32_t n = 0; n < level; n++) {
				for (uint32_t k = 0; k < len; k++) {
					code = (code << 1) | !!(data[k] & bitMask);
				}

				bitMask >>= 1;
			}

			return code;
		}

		// From: https://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers
		inline uint32_t Log2(uint32_t value) {
			assert(value != 0);
#if defined(_MSC_VER)
			unsigned long index;
#if _MSC_VER <= 1200
			_asm {
				BSR EAX, value;
				MOV index, EAX;
			}
#else
			_BitScanReverse(&index, value);
#endif

			return index;
#else
			return sizeof(uint32_t) * 8 - 1 - __builtin_clz(value);
#endif
			/*
			static const uint32_t tab32[32] = {
				0,  9,  1, 10, 13, 21,  2, 29,
				11, 14, 16, 18, 22, 25,  3, 30,
				8, 12, 20, 28, 15, 17, 24,  7,
				19, 27, 23,  6, 26,  5,  4, 31 };
			value |= value >> 1;
			value |= value >> 2;
			value |= value >> 4;
			value |= value >> 8;
			value |= value >> 16;
			return tab32[(uint32_t)(value * 0x07C4ACDD) >> 27];*/
		}

		inline uint32_t Log2(uint64_t value) {
			assert(value != 0);
#if defined(_MSC_VER)
#if !defined(_M_AMD64)
			/*
			const uint32_t tab64[64] = {
				63,  0, 58,  1, 59, 47, 53,  2,
				60, 39, 48, 27, 54, 33, 42,  3,
				61, 51, 37, 40, 49, 18, 28, 20,
				55, 30, 34, 11, 43, 14, 22,  4,
				62, 57, 46, 52, 38, 26, 32, 41,
				50, 36, 17, 19, 29, 10, 13, 21,
				56, 45, 25, 31, 35, 16,  9, 12,
				44, 24, 15,  8, 23,  7,  6,  5 };

			value |= value >> 1;
			value |= value >> 2;
			value |= value >> 4;
			value |= value >> 8;
			value |= value >> 16;
			value |= value >> 32;
			return tab64[((uint64_t)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
			*/
			uint32_t highPart = (uint32_t)((value >> 31) >> 1);
			return highPart != 0 ? Log2(highPart) + 32 : Log2((uint32_t)(value & 0xffffffff));
#else
			unsigned long index;
			_BitScanReverse64(&index, value);
			return index;
#endif
#else
			return sizeof(uint64_t) * 8 - 1 - __builtin_clzll(value);
#endif
		}

		inline uint32_t Log2x(uint32_t value) {
			assert(value != 0);
#if defined(_MSC_VER)
			unsigned long index;
#if _MSC_VER <= 1200
			_asm {
				BSF EAX, value;
				MOV index, EAX;
			}
#else
			_BitScanForward(&index, value);
#endif

			return index;
#else
			return __builtin_ctz(value);
#endif
		}

		inline uint32_t Log2x(uint64_t value) {
			assert(value != 0);
#if defined(_MSC_VER)
#if !defined(_M_AMD64)
			uint32_t lowPart = (uint32_t)(value & 0xFFFFFFFF);
			return lowPart == 0 ? Log2x((uint32_t)((value >> 31) >> 1)) + 32 : Log2x(lowPart);
#else
			unsigned long index;
			_BitScanForward64(&index, value);
			return index;
#endif
#else
			return __builtin_ctzll(value);
#endif
		}
	}
}
