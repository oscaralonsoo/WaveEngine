////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __MATH_SIMD_H__
#define __MATH_SIMD_H__


#include <NsCore/Noesis.h>


#define NS_ENABLE_SSE 1
#define NS_ENABLE_AVX 1
#define NS_ENABLE_NEON 1
#define NS_ENABLE_WASM 1


// Neon emulation with https://github.com/simd-everywhere/simde
//#define SIMDE_ENABLE_NATIVE_ALIASES
//#include <cmath>
//#include "simde/arm/neon.h"
//#define __ARM_NEON
//#define __aarch64__

// Wasm Simd128 emulation with https://github.com/simd-everywhere/simde
//#define SIMDE_ENABLE_NATIVE_ALIASES
//#include <cmath>
//#include "simde/wasm/simd128.h"
//#define __wasm_simd128__


#if NS_ENABLE_SSE && (defined(__SSE2__) || (defined(_MSC_VER) && (_M_AMD64 || _M_IX86_FP >= 2)))

#define NS_HAVE_SIMD128
#include <xmmintrin.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// SSE implementation using 128-bit SIMD intrinsics (__m128)
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Noesis
{

typedef __m128 Simd128;
typedef __m128 Simd128b;

inline Simd128 SimdLoad4(const float* vv)
{
    return _mm_loadu_ps(vv);
}

inline Simd128 SimdLoad4A(const float* vv)
{
    return _mm_load_ps(vv);
}

inline Simd128 SimdLoad4(float x, float y, float z, float w)
{
    return _mm_set_ps(w, z, y, x);
}

inline Simd128 SimdLoad4(float x)
{
    return _mm_set1_ps(x);
}

inline Simd128b SimdCmpEq(Simd128 a, Simd128 b)
{
    return _mm_cmpeq_ps(a, b);
}

inline Simd128b SimdCmpLt(Simd128 a, Simd128 b)
{
    return _mm_cmplt_ps(a, b);
}

inline bool SimdAll(Simd128b a)
{
    return _mm_movemask_ps(a) == 0xF;
}

inline bool SimdAny(Simd128b a)
{
    return _mm_movemask_ps(a) != 0;
}

}
#endif

#if NS_ENABLE_NEON && (defined(__ARM_NEON) || defined(__ARM_NEON__))

#define NS_HAVE_SIMD128

#ifndef SIMDE_ARM_NEON_H
#include <arm_neon.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// NEON implementation using 128-bit ARM SIMD intrinsics (float32x4_t / uint32x4_t)
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Noesis
{

typedef float32x4_t Simd128;
typedef uint32x4_t Simd128b;

inline Simd128 SimdLoad4(const float* vv)
{
    return vld1q_f32(vv);
}

inline Simd128 SimdLoad4A(const float* vv)
{
    return vld1q_f32(vv);
}

inline Simd128 SimdLoad4(float x, float y, float z, float w)
{
    const float32_t val[4] = {x, y, z, w};
    return vld1q_f32(val);
}

inline Simd128 SimdLoad4(float x)
{
    return vdupq_n_f32(x);
}

inline Simd128b SimdCmpEq(Simd128 a, Simd128 b)
{
    return vceqq_f32(a, b);
}

inline Simd128b SimdCmpLt(Simd128 a, Simd128 b)
{
    return vcltq_f32(a, b);
}

inline bool SimdAll(Simd128b a)
{
  #if defined(__aarch64__)
    return vminvq_u32(a) == 0xFFFFFFFF;
  #else
    return (a[0] & a[1] & a[2] & a[3]) == 0xFFFFFFFF;
  #endif
}

inline bool SimdAny(Simd128b a)
{
  #if defined(__aarch64__)
    return vmaxvq_u32(a) != 0;
  #else
    return (a[0] | a[1] | a[2] | a[3]) != 0;
  #endif
}

}
#endif

#if NS_ENABLE_WASM && defined(__wasm_simd128__)

#define NS_HAVE_SIMD128

#ifndef SIMDE_WASM_SIMD128_H
#include <wasm_simd128.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// WebAssembly SIMD implementation using 128-bit v128 vectors
// Uses wasm_simd128.h intrinsics (f32x4 / i8x16 bitmask) for lane-wise operations and masks
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Noesis
{

typedef v128_t Simd128;
typedef v128_t Simd128b;

inline Simd128 SimdLoad4(const float* vv)
{
    return wasm_v128_load(vv);
}

inline Simd128 SimdLoad4A(const float* vv)
{
    return wasm_v128_load(vv);
}

inline Simd128 SimdLoad4(float x, float y, float z, float w)
{
    return wasm_f32x4_make(x, y, z, w);
}

inline Simd128 SimdLoad4(float x)
{
    return wasm_f32x4_splat(x);
}

inline Simd128b SimdCmpEq(Simd128 a, Simd128 b)
{
    return wasm_f32x4_eq(a, b);
}

inline Simd128b SimdCmpLt(Simd128 a, Simd128 b)
{
    return wasm_f32x4_lt(a, b);
}

inline bool SimdAll(Simd128b a)
{
    return wasm_i32x4_bitmask(a) == 0xF;
}

inline bool SimdAny(Simd128b a)
{
    return wasm_i32x4_bitmask(a) != 0;
}

}
#endif

#if NS_ENABLE_AVX && (defined(__AVX__) || defined(__AVX2__))

#define NS_HAVE_SIMD256
#include <immintrin.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// AVX implementation using 256-bit SIMD intrinsics (__m256)
// Supports 8x float vectors and 8-lane comparison/mask operations
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Noesis
{

typedef __m256 Simd256;

inline Simd256 SimdLoad8(Simd128 a, Simd128 b)
{
    return _mm256_set_m128(b, a);
}

inline Simd256 SimdLoad8(const float* vv)
{
    return _mm256_loadu_ps(vv);
}

inline Simd256 SimdLoad8A(const float* vv)
{
    return _mm256_load_ps(vv);
}

inline Simd256 SimdLoad8(float x0, float x1, float x2, float x3, float x4, float x5, float x6, float x7)
{
    return _mm256_set_ps(x7, x6, x5, x4, x3, x2, x1, x0);
}

inline Simd256 SimdLoad8(float x)
{
    return _mm256_set1_ps(x);
}

inline Simd256 SimdCmpEq(Simd256 a, Simd256 b)
{
    return _mm256_cmp_ps(a, b, _CMP_EQ_OQ);
}

inline Simd256 SimdCmpLt(Simd256 a, Simd256 b)
{
    return _mm256_cmp_ps(a, b, _CMP_LT_OQ);
}

inline bool SimdAll(Simd256 a)
{
    return _mm256_movemask_ps(a) == 0xFF;
}

inline bool SimdAny(Simd256 a)
{
    return _mm256_movemask_ps(a) != 0;
}

inline bool SimdAllHalf(Simd256 a)
{
    // True if all 4 lanes in either the lower or upper are set
    uint32_t mask = (uint32_t)_mm256_movemask_ps(a);
    return mask >= 0xF0 || (mask & 0xF) == 0xF;
}
}
#endif


#ifndef NS_HAVE_SIMD128

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scalar 128-bit implementation using 4 floats
// Provides 4-lane operations without SIMD, for platforms lacking hardware SIMD support
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Noesis
{

struct Simd128 { float v[4]; };
struct Simd128b { bool v[4]; };

inline Simd128 SimdLoad4(const float* vv)
{
    return Simd128{ vv[0], vv[1], vv[2], vv[3] };
}

inline Simd128 SimdLoad4A(const float* vv)
{
    return Simd128{ vv[0], vv[1], vv[2], vv[3] };
}

inline Simd128 SimdLoad4(float x, float y, float z, float w)
{
    return Simd128{ x, y, z, w };
}

inline Simd128 SimdLoad4(float x)
{
    return Simd128{ x, x, x, x };
}

inline Simd128b SimdCmpEq(Simd128 a, Simd128 b)
{
    return Simd128b{ a.v[0] == b.v[0], a.v[1] == b.v[1], a.v[2] == b.v[2], a.v[3] == b.v[3] };
}

inline Simd128b SimdCmpLt(Simd128 a, Simd128 b)
{
    return Simd128b{ a.v[0] < b.v[0], a.v[1] < b.v[1], a.v[2] < b.v[2], a.v[3] < b.v[3] };
}

inline bool SimdAll(Simd128b a)
{
    return a.v[0] && a.v[1] && a.v[2] && a.v[3];
}

inline bool SimdAny(Simd128b a)
{
    return a.v[0] || a.v[1] || a.v[2] || a.v[3];
}

}
#endif

#ifndef NS_HAVE_SIMD256

////////////////////////////////////////////////////////////////////////////////////////////////////
// Portable 256-bit SIMD implementation using two 128-bit Simd128 lanes
// Combines two 128-bit vectors to emulate 8-lane operations on platforms without 256-bit SIMD
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Noesis
{

struct Simd256 { Simd128 v[2]; };
struct Simd256b { Simd128b v[2]; };

inline Simd256 SimdLoad8(Simd128 a, Simd128 b)
{
    return Simd256{ a, b };
}

inline Simd256 SimdLoad8(const float* vv)
{
    return Simd256{ SimdLoad4(vv), SimdLoad4(vv + 4) };
}

inline Simd256 SimdLoad8A(const float* vv)
{
    return Simd256{ SimdLoad4A(vv), SimdLoad4A(vv + 4) };
}

inline Simd256 SimdLoad8(float x0, float x1, float x2, float x3, float x4, float x5, float x6, float x7)
{
    return Simd256{ SimdLoad4(x0, x1, x2, x3), SimdLoad4(x4, x5, x6, x7) };
}

inline Simd256 SimdLoad8(float x)
{
    return Simd256{ SimdLoad4(x), SimdLoad4(x) };
}

inline Simd256b SimdCmpEq(const Simd256& a, const Simd256& b)
{
    return Simd256b{ SimdCmpEq(a.v[0], b.v[0]), SimdCmpEq(a.v[1], b.v[1]) };
}

inline Simd256b SimdCmpLt(const Simd256& a, const Simd256& b)
{
    return Simd256b{ SimdCmpLt(a.v[0], b.v[0]), SimdCmpLt(a.v[1], b.v[1]) };
}

inline bool SimdAll(const Simd256b& a)
{
    return SimdAll(a.v[0]) && SimdAll(a.v[1]);
}

inline bool SimdAny(const Simd256b& a)
{
    return SimdAny(a.v[0]) || SimdAny(a.v[1]);
}

inline bool SimdAllHalf(const Simd256b& a)
{
    return SimdAll(a.v[0]) || SimdAll(a.v[1]);
}

}

#endif

#endif
