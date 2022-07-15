// TShaderMacro.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-10
//

#pragma once
#include "../../Core/Interface/IType.h"
#include "../Interface/IRender.h"

#ifdef _MSC_VER
#pragma warning(disable:4305) // 1.0 to double
#pragma warning(disable:4244)
#endif

#define USE_SWIZZLE

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace PaintsNow {

#define IF (f)
#define ELSE
#define ENDIF
	const float PI = 3.1415926f;
	const float GAMMA = 2.2f;
	typedef uint32_t uint;
	typedef UInt2 uint2;
	typedef UInt3 uint3;
	typedef UInt4 uint4;
	typedef Int2 int2;
	typedef Int3 int3;
	typedef Int4 int4;
	typedef Float4 float4;
	typedef Float3 float3;
	typedef Float2 float2;
	typedef MatrixFloat3x3 float3x3;
	typedef MatrixFloat4x4 float4x4;
	typedef IRender::Resource::TextureDescription sampler2D;
	typedef IRender::Resource::TextureDescription sampler2DCube;

#define make_float3x3(a11, a12, a13, a21, a22, a23, a31, a32, a33) float3x3()
#define make_float4x4(a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34, a41, a42, a43, a44) float4x4()

	// just prototypes to make compiler happy. No actual effects.
#if defined(_MSC_VER) && _MSC_VER <= 1200
#define FILTER_FUNCTION(func) \
	template <class T> \
	T func(T input) { return input; } \
	template <class T> \
	TType2<T> func(const TVector<T, 2>& input) { return input;  } \
	template <class T> \
	TType3<T> func(const TVector<T, 3>& input) { return input;  } \
	template <class T> \
	TType4<T> func(const TVector<T, 4>& input) { return input;  }
#else
#define FILTER_FUNCTION(func) \
	template <class T> \
	T func(const T& input) { return input; } \
	template <class T> \
	TType2<T> func(const TVector<T, 2>& input) { return input;  } \
	template <class T> \
	TType3<T> func(const TVector<T, 3>& input) { return input;  } \
	template <class T> \
	TType4<T> func(const TVector<T, 4>& input) { return input;  }
#endif

	FILTER_FUNCTION(normalize);
	FILTER_FUNCTION(saturate);
	FILTER_FUNCTION(radians);
	FILTER_FUNCTION(degrees);
	FILTER_FUNCTION(sin);
	FILTER_FUNCTION(cos);
	FILTER_FUNCTION(tan);
	FILTER_FUNCTION(asin);
	FILTER_FUNCTION(acos);
	FILTER_FUNCTION(atan);
	FILTER_FUNCTION(exp);
	FILTER_FUNCTION(log);
	FILTER_FUNCTION(exp2);
	FILTER_FUNCTION(log2);
	FILTER_FUNCTION(sqrt);
	FILTER_FUNCTION(inversesqrt);
	FILTER_FUNCTION(abs);
	FILTER_FUNCTION(sign);
	FILTER_FUNCTION(floor);
	FILTER_FUNCTION(ceil);
	FILTER_FUNCTION(round);
	FILTER_FUNCTION(fract);

#define FILTER_FUNCTION_RET_T(func) \
	template <class T> \
	T func(const TVector<T, 2>& input) { return T(0);  } \
	template <class T> \
	T func(const TVector<T, 3>& input) { return T(0);  } \
	template <class T> \
	T func(const TVector<T, 4>& input) { return T(0);  }

	FILTER_FUNCTION_RET_T(length);

#define FILTER_FUNCTION_TWO(func) \
	template <class T> \
	T func(T input, float other) { return input;  } \
	template <class T> \
	TType2<T> func(const TVector<T, 2>& input, const TVector<T, 2>& other) { return input;  } \
	template <class T> \
	TType3<T> func(const TVector<T, 3>& input, const TVector<T, 3>& other) { return input;  } \
	template <class T> \
	TType4<T> func(const TVector<T, 4>& input, const TVector<T, 4>& other) { return input;  }

	FILTER_FUNCTION_TWO(atan);
	FILTER_FUNCTION_TWO(pow);
	FILTER_FUNCTION_TWO(mod);

	FILTER_FUNCTION_TWO(_min);
	FILTER_FUNCTION_TWO(_max);

#define min _min
#define max _max

	FILTER_FUNCTION_TWO(step);
	FILTER_FUNCTION_TWO(distance);
	FILTER_FUNCTION_TWO(reflect);
	FILTER_FUNCTION_TWO(refract);

#define FILTER_FUNCTION_TWO_RET_T(func) \
	template <class T> \
	T func(const TVector<T, 2>& input, const TVector<T, 2>& other) { return T(0);  } \
	template <class T> \
	T func(const TVector<T, 3>& input, const TVector<T, 3>& other) { return T(0);  } \
	template <class T> \
	T func(const TVector<T, 4>& input, const TVector<T, 4>& other) { return T(0);  }

	FILTER_FUNCTION_TWO_RET_T(dot);
	template <class T>
	TType3<T> cross(const TVector<T, 3>& input, const TVector<T, 3>& other) {
		return input;
	}

#define FILTER_FUNCTION_THREE(func) \
	template <class T> \
	T func(T input, T other, T last) { return input;  } \
	template <class T> \
	TType2<T> func(const TVector<T, 2>& input, const TVector<T, 2>& other, const TVector<T, 2>& last) { return input;  } \
	template <class T> \
	TType3<T> func(const TVector<T, 3>& input, const TVector<T, 3>& other, const TVector<T, 2>& last) { return input;  } \
	template <class T> \
	TType4<T> func(const TVector<T, 4>& input, const TVector<T, 4>& other, const TVector<T, 2>& last) { return input;  }

	FILTER_FUNCTION_THREE(smoothstep);

	template <class T>
	inline TType4<T> projection(const TVector<T, 4>& proj, const TVector<T, 3>& v) {
		return TType4<T>(proj[0] * v[0], proj[1] * v[1], proj[2] * v[2] + proj[3], -v[2]);
	}

	template <class T>
	inline TType3<T> unprojection(const TVector<T, 4>& unproj, const TVector<T, 3>& v) {
		T nz = unproj[3] / (unproj[2] + v[2]);
		return TType3<T>(unproj[0] * v[0] * nz, unproj[1] * v[1] * nz, -nz);
	}

	template <class T>
	TType4<T> texture(const IShader::BindTexture& tex, const TVector<T, 2>& v, T bias = 0) {
		return TType4<T>(0, 0, 0, 0);
	}

	template <class T>
	TType4<T> texture(const IShader::BindTexture& tex, const TVector<T, 3>& v, T bias = 0) {
		return TType4<T>(0, 0, 0, 0);
	}

	template <class T>
	TType4<T> texture(const IShader::BindTexture& tex, const TVector<T, 4>& v, T bias = 0) {
		return TType4<T>(0, 0, 0, 0);
	}

	template <class T>
	TType4<T> textureLod(const IShader::BindTexture& tex, const TVector<T, 2>& v, T lod = 0) {
		return TType4<T>(0, 0, 0, 0);
	}

	template <class T>
	TType4<T> textureLod(const IShader::BindTexture& tex, const TVector<T, 3>& v, T lod = 0) {
		return TType4<T>(0, 0, 0, 0);
	}

	template <class T>
	TType4<T> textureLod(const IShader::BindTexture& tex, const TVector<T, 4>& v, T lod = 0) {
		return TType4<T>(0, 0, 0, 0);
	}

	template <class T>
	float textureShadow(const IShader::BindTexture& tex, const TVector<T, 3>& v) {
		return 0;
	}

	inline float4 imageLoad(const IShader::BindTexture& tex, const Int2& v) {
		return float4();
	}

	inline float4 imageLoad(const IShader::BindTexture& tex, const Int3& v) {
		return float4();
	}

	inline float4 imageStore(const IShader::BindTexture& tex, const Int2& v) {
		return float4();
	}

	inline float4 imageStore(const IShader::BindTexture& tex, const Int3& v) {
		return float4();
	}

	template <class T>
	T atomicAdd(T& mem, T data) {
		return data;
	}

	template <class T>
	T imageAtomicAdd(const IShader::BindTexture& tex, const Int2& v, T data) {
		return data;
	}

	template <class T>
	T imageAtomicAdd(const IShader::BindTexture& tex, const Int3& v, T data) {
		return data;
	}

	template <class T>
	T atomicMin(T& mem, T data) {
		return data;
	}

	template <class T>
	T imageAtomicMin(const IShader::BindTexture& tex, const Int2& v, T data) {
		return data;
	}

	template <class T>
	T imageAtomicMin(const IShader::BindTexture& tex, const Int3& v, T data) {
		return data;
	}

	template <class T>
	T atomicMax(T& mem, T data) {
		return data;
	}

	template <class T>
	T imageAtomicMax(const IShader::BindTexture& tex, const Int2& v, T data) {
		return data;
	}

	template <class T>
	T imageAtomicMax(const IShader::BindTexture& tex, const Int3& v, T data) {
		return data;
	}

	template <class T>
	T atomicAnd(T& mem, T data) {
		return data;
	}

	template <class T>
	T imageAtomicAnd(const IShader::BindTexture& tex, const Int2& v, T data) {
		return data;
	}

	template <class T>
	T imageAtomicAnd(const IShader::BindTexture& tex, const Int3& v, T data) {
		return data;
	}

	template <class T>
	T atomicOr(T& mem, T data) {
		return data;
	}

	template <class T>
	T imageAtomicOr(const IShader::BindTexture& tex, const Int2& v, T data) {
		return data;
	}

	template <class T>
	T imageAtomicOr(const IShader::BindTexture& tex, const Int3& v, T data) {
		return data;
	}

	template <class T>
	T atomicXor(T& mem, T data) {
		return data;
	}

	template <class T>
	T imageAtomicXor(const IShader::BindTexture& tex, const Int2& v, T data) {
		return data;
	}

	template <class T>
	T imageAtomicXor(const IShader::BindTexture& tex, const Int3& v, T data) {
		return data;
	}

	template <class T>
	T atomicExchange(T& mem, T data) {
		return data;
	}

	template <class T>
	T imageAtomicExchange(const IShader::BindTexture& tex, const Int2& v, T data) {
		return data;
	}

	template <class T>
	T imageAtomicExchange(const IShader::BindTexture& tex, const Int3& v, T data) {
		return data;
	}

	template <class T>
	T atomicCompSwap(T& mem, T compare, T data) {
		return data;
	}

	template <class T>
	T imageAtomicCompSwap(const IShader::BindTexture& tex, const Int2& v, T compare, T data) {
		return data;
	}

	template <class T>
	T imageAtomicCompSwap(const IShader::BindTexture& tex, const Int3& v, T compare, T data) {
		return data;
	}

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <class T>
	void clip(T value) {}

	template <class T, class D>
	T lerp(T r, T v, D c) {
		return T();
	}

	template <class T>
	T clamp(T r, T v, T c) {
		return T();
	}
#else
	template <class T>
	void clip(const T& value) {}

	template <class T, class D>
	T lerp(const T& r, const T& v, const D& c) {
		return T();
	}

	template <class T>
	T clamp(const T& r, const T& v, const T& c) {
		return T();
	}
#endif

	template <class T>
	TType2<T> lerp(const TVector<T, 2>& r, const TVector<T, 2>& v, T value) {
		return TType2<T>(0, 0);
	}

	template <class T>
	TType3<T> lerp(const TVector<T, 3>& r, const TVector<T, 3>& v, T value) {
		return TType3<T>(0, 0, 0);
	}

	template <class T>
	TType4<T> lerp(const TVector<T, 4>& r, const TVector<T, 4>& v, T value) {
		return TType4<T>(0, 0, 0, 0);
	}

	template <class T, size_t n, size_t m>
	TMatrix<T, n, n> mult_mat(const TMatrix<T, n, m>& mat, const TMatrix<T, m, n>& rhs) {
		return mat;
	}

	template <class T>
	TType3<T> mult_vec(const MatrixFloat3x3& mat, const TVector<T, 3>& value) {
		return value;
	}

	template <class T>
	TType4<T> mult_vec(const MatrixFloat4x4& mat, const TVector<T, 4>& value) {
		return value;
	}

	template <class T>
	TType4<T> _EncodeNormalMaterial(const TVector<T, 4>& input) {
		return input;
	}

	template <class T>
	TType4<T> _DecodeNormalMaterial(const TVector<T, 4>& output) {
		return output;
	}

	inline void barrier() {}
	inline void memoryBarrier() {}
	inline void memoryBarrierShared() {}
	inline void groupMemoryBarrier() {}

#define ATOMIC_OP(nint) \
	inline nint atomicAdd(nint&, nint) { return 0; } \
	inline nint atomicMin(nint&, nint) { return 0; } \
	inline nint atomicMax(nint&, nint) { return 0; } \
	inline nint atomicAnd(nint&, nint) { return 0; } \
	inline nint atomicOr(nint&, nint) { return 0; } \
	inline nint atomicXor(nint&, nint) { return 0; } \
	inline nint atomicExchange(nint&, nint) { return 0; } \
	inline nint atomicCompSwap(nint&, nint, nint) { return 0; } \

	ATOMIC_OP(int32_t)
	ATOMIC_OP(uint32_t)
}

#ifndef USE_SHADER_CODE_DEBUG
#define UnifyShaderCode(code) \
	#code; \
	/* grammar checking */ \
	if (false) { \
		code; \
	} \
	return "";
#else
#define UnifyShaderCode(code) \
	code;
#endif

// two components

#define x data[0]
#define y data[1]
#define z data[2]
#define w data[3]

#define xx _xx()
#define xy _xy()
#define xz _xz()
#define xw _xw()
#define yx _yx()
#define yy _yy()
#define yz _yz()
#define yw _yw()
#define zx _zx()
#define zy _zy()
#define zz _zz()
#define zw _zw()
#define wx _wx()
#define wy _wy()
#define wz _wz()
#define ww _ww()

// three components
#define xxx _xxx()
#define xxy _xxy()
#define xxz _xxz()
#define xxw _xxw()
#define xyx _xyx()
#define xyy _xyy()
#define xyz _xyz()
#define xyw _xyw()
#define xzx _xzx()
#define xzy _xzy()
#define xzz _xzz()
#define xzw _xzw()
#define xwx _xwx()
#define xwy _xwy()
#define xwz _xwz()
#define xww _xww()
#define yxx _yxx()
#define yxy _yxy()
#define yxz _yxz()
#define yxw _yxw()
#define yyx _yyx()
#define yyy _yyy()
#define yyz _yyz()
#define yyw _yyw()
#define yzx _yzx()
#define yzy _yzy()
#define yzz _yzz()
#define yzw _yzw()
#define ywx _ywx()
#define ywy _ywy()
#define ywz _ywz()
#define yww _yww()
#define zxx _zxx()
#define zxy _zxy()
#define zxz _zxz()
#define zxw _zxw()
#define zyx _zyx()
#define zyy _zyy()
#define zyz _zyz()
#define zyw _zyw()
#define zzx _zzx()
#define zzy _zzy()
#define zzz _zzz()
#define zzw _zzw()
#define zwx _zwx()
#define zwy _zwy()
#define zwz _zwz()
#define zww _zww()
#define wxx _wxx()
#define wxy _wxy()
#define wxz _wxz()
#define wxw _wxw()
#define wyx _wyx()
#define wyy _wyy()
#define wyz _wyz()
#define wyw _wyw()
#define wzx _wzx()
#define wzy _wzy()
#define wzz _wzz()
#define wzw _wzw()
#define wwx _wwx()
#define wwy _wwy()
#define wwz _wwz()
#define www _www()

// four components
#define xxxx _xxxx()
#define xxxy _xxxy()
#define xxxz _xxxz()
#define xxxw _xxxw()
#define xxyx _xxyx()
#define xxyy _xxyy()
#define xxyz _xxyz()
#define xxyw _xxyw()
#define xxzx _xxzx()
#define xxzy _xxzy()
#define xxzz _xxzz()
#define xxzw _xxzw()
#define xxwx _xxwx()
#define xxwy _xxwy()
#define xxwz _xxwz()
#define xxww _xxww()
#define xyxx _xyxx()
#define xyxy _xyxy()
#define xyxz _xyxz()
#define xyxw _xyxw()
#define xyyx _xyyx()
#define xyyy _xyyy()
#define xyyz _xyyz()
#define xyyw _xyyw()
#define xyzx _xyzx()
#define xyzy _xyzy()
#define xyzz _xyzz()
#define xyzw _xyzw()
#define xywx _xywx()
#define xywy _xywy()
#define xywz _xywz()
#define xyww _xyww()
#define xzxx _xzxx()
#define xzxy _xzxy()
#define xzxz _xzxz()
#define xzxw _xzxw()
#define xzyx _xzyx()
#define xzyy _xzyy()
#define xzyz _xzyz()
#define xzyw _xzyw()
#define xzzx _xzzx()
#define xzzy _xzzy()
#define xzzz _xzzz()
#define xzzw _xzzw()
#define xzwx _xzwx()
#define xzwy _xzwy()
#define xzwz _xzwz()
#define xzww _xzww()
#define xwxx _xwxx()
#define xwxy _xwxy()
#define xwxz _xwxz()
#define xwxw _xwxw()
#define xwyx _xwyx()
#define xwyy _xwyy()
#define xwyz _xwyz()
#define xwyw _xwyw()
#define xwzx _xwzx()
#define xwzy _xwzy()
#define xwzz _xwzz()
#define xwzw _xwzw()
#define xwwx _xwwx()
#define xwwy _xwwy()
#define xwwz _xwwz()
#define xwww _xwww()
#define yxxx _yxxx()
#define yxxy _yxxy()
#define yxxz _yxxz()
#define yxxw _yxxw()
#define yxyx _yxyx()
#define yxyy _yxyy()
#define yxyz _yxyz()
#define yxyw _yxyw()
#define yxzx _yxzx()
#define yxzy _yxzy()
#define yxzz _yxzz()
#define yxzw _yxzw()
#define yxwx _yxwx()
#define yxwy _yxwy()
#define yxwz _yxwz()
#define yxww _yxww()
#define yyxx _yyxx()
#define yyxy _yyxy()
#define yyxz _yyxz()
#define yyxw _yyxw()
#define yyyx _yyyx()
#define yyyy _yyyy()
#define yyyz _yyyz()
#define yyyw _yyyw()
#define yyzx _yyzx()
#define yyzy _yyzy()
#define yyzz _yyzz()
#define yyzw _yyzw()
#define yywx _yywx()
#define yywy _yywy()
#define yywz _yywz()
#define yyww _yyww()
#define yzxx _yzxx()
#define yzxy _yzxy()
#define yzxz _yzxz()
#define yzxw _yzxw()
#define yzyx _yzyx()
#define yzyy _yzyy()
#define yzyz _yzyz()
#define yzyw _yzyw()
#define yzzx _yzzx()
#define yzzy _yzzy()
#define yzzz _yzzz()
#define yzzw _yzzw()
#define yzwx _yzwx()
#define yzwy _yzwy()
#define yzwz _yzwz()
#define yzww _yzww()
#define ywxx _ywxx()
#define ywxy _ywxy()
#define ywxz _ywxz()
#define ywxw _ywxw()
#define ywyx _ywyx()
#define ywyy _ywyy()
#define ywyz _ywyz()
#define ywyw _ywyw()
#define ywzx _ywzx()
#define ywzy _ywzy()
#define ywzz _ywzz()
#define ywzw _ywzw()
#define ywwx _ywwx()
#define ywwy _ywwy()
#define ywwz _ywwz()
#define ywww _ywww()
#define zxxx _zxxx()
#define zxxy _zxxy()
#define zxxz _zxxz()
#define zxxw _zxxw()
#define zxyx _zxyx()
#define zxyy _zxyy()
#define zxyz _zxyz()
#define zxyw _zxyw()
#define zxzx _zxzx()
#define zxzy _zxzy()
#define zxzz _zxzz()
#define zxzw _zxzw()
#define zxwx _zxwx()
#define zxwy _zxwy()
#define zxwz _zxwz()
#define zxww _zxww()
#define zyxx _zyxx()
#define zyxy _zyxy()
#define zyxz _zyxz()
#define zyxw _zyxw()
#define zyyx _zyyx()
#define zyyy _zyyy()
#define zyyz _zyyz()
#define zyyw _zyyw()
#define zyzx _zyzx()
#define zyzy _zyzy()
#define zyzz _zyzz()
#define zyzw _zyzw()
#define zywx _zywx()
#define zywy _zywy()
#define zywz _zywz()
#define zyww _zyww()
#define zzxx _zzxx()
#define zzxy _zzxy()
#define zzxz _zzxz()
#define zzxw _zzxw()
#define zzyx _zzyx()
#define zzyy _zzyy()
#define zzyz _zzyz()
#define zzyw _zzyw()
#define zzzx _zzzx()
#define zzzy _zzzy()
#define zzzz _zzzz()
#define zzzw _zzzw()
#define zzwx _zzwx()
#define zzwy _zzwy()
#define zzwz _zzwz()
#define zzww _zzww()
#define zwxx _zwxx()
#define zwxy _zwxy()
#define zwxz _zwxz()
#define zwxw _zwxw()
#define zwyx _zwyx()
#define zwyy _zwyy()
#define zwyz _zwyz()
#define zwyw _zwyw()
#define zwzx _zwzx()
#define zwzy _zwzy()
#define zwzz _zwzz()
#define zwzw _zwzw()
#define zwwx _zwwx()
#define zwwy _zwwy()
#define zwwz _zwwz()
#define zwww _zwww()
#define wxxx _wxxx()
#define wxxy _wxxy()
#define wxxz _wxxz()
#define wxxw _wxxw()
#define wxyx _wxyx()
#define wxyy _wxyy()
#define wxyz _wxyz()
#define wxyw _wxyw()
#define wxzx _wxzx()
#define wxzy _wxzy()
#define wxzz _wxzz()
#define wxzw _wxzw()
#define wxwx _wxwx()
#define wxwy _wxwy()
#define wxwz _wxwz()
#define wxww _wxww()
#define wyxx _wyxx()
#define wyxy _wyxy()
#define wyxz _wyxz()
#define wyxw _wyxw()
#define wyyx _wyyx()
#define wyyy _wyyy()
#define wyyz _wyyz()
#define wyyw _wyyw()
#define wyzx _wyzx()
#define wyzy _wyzy()
#define wyzz _wyzz()
#define wyzw _wyzw()
#define wywx _wywx()
#define wywy _wywy()
#define wywz _wywz()
#define wyww _wyww()
#define wzxx _wzxx()
#define wzxy _wzxy()
#define wzxz _wzxz()
#define wzxw _wzxw()
#define wzyx _wzyx()
#define wzyy _wzyy()
#define wzyz _wzyz()
#define wzyw _wzyw()
#define wzzx _wzzx()
#define wzzy _wzzy()
#define wzzz _wzzz()
#define wzzw _wzzw()
#define wzwx _wzwx()
#define wzwy _wzwy()
#define wzwz _wzwz()
#define wzww _wzww()
#define wwxx _wwxx()
#define wwxy _wwxy()
#define wwxz _wwxz()
#define wwxw _wwxw()
#define wwyx _wwyx()
#define wwyy _wwyy()
#define wwyz _wwyz()
#define wwyw _wwyw()
#define wwzx _wwzx()
#define wwzy _wwzy()
#define wwzz _wwzz()
#define wwzw _wwzw()
#define wwwx _wwwx()
#define wwwy _wwwy()
#define wwwz _wwwz()
#define wwww _wwww()

