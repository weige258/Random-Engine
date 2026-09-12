#pragma once

#include "SIMDBase.hpp"

#if defined(_MSC_VER)
    #include <intrin.h>
#else
    #if defined(RSIMD_AVX2) || defined(RSIMD_AVX)
        #include <immintrin.h>
    #elif defined(RSIMD_SSE42)
        #include <nmmintrin.h>
    #elif defined(RSIMD_SSE3)
        #include <pmmintrin.h>
    #elif defined(RSIMD_SSE2)
        #include <emmintrin.h>
    #endif
#endif

#include <cmath>

namespace RandomEngine::Platform::SIMD
{

// =========================================================================
// float 实现
// =========================================================================

#if defined(RSIMD_AVX)

RSIMD_FORCEINLINE auto Loadf(const float* ptr) { return _mm256_load_ps(ptr); }
RSIMD_FORCEINLINE auto LoadUf(const float* ptr) { return _mm256_loadu_ps(ptr); }
RSIMD_FORCEINLINE void Storef(float* ptr, __m256 vec) { _mm256_store_ps(ptr, vec); }
RSIMD_FORCEINLINE void StoreUf(float* ptr, __m256 vec) { _mm256_storeu_ps(ptr, vec); }
RSIMD_FORCEINLINE auto Set1f(float val) { return _mm256_set1_ps(val); }
RSIMD_FORCEINLINE auto Zerof() { return _mm256_setzero_ps(); }
RSIMD_FORCEINLINE auto Setf(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7) {
    return _mm256_set_ps(v7, v6, v5, v4, v3, v2, v1, v0);
}

RSIMD_FORCEINLINE auto Addf(__m256 a, __m256 b) { return _mm256_add_ps(a, b); }
RSIMD_FORCEINLINE auto Subf(__m256 a, __m256 b) { return _mm256_sub_ps(a, b); }
RSIMD_FORCEINLINE auto Mulf(__m256 a, __m256 b) { return _mm256_mul_ps(a, b); }
RSIMD_FORCEINLINE auto Divf(__m256 a, __m256 b) { return _mm256_div_ps(a, b); }
RSIMD_FORCEINLINE auto Negf(__m256 a) { return _mm256_sub_ps(_mm256_setzero_ps(), a); }
RSIMD_FORCEINLINE auto Absf(__m256 a) {
    return _mm256_and_ps(a, _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF)));
}

RSIMD_FORCEINLINE auto FMAddf(__m256 a, __m256 b, __m256 c) {
#if defined(RSIMD_FMA)
    return _mm256_fmadd_ps(a, b, c);
#else
    return _mm256_add_ps(_mm256_mul_ps(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FMSubf(__m256 a, __m256 b, __m256 c) {
#if defined(RSIMD_FMA)
    return _mm256_fmsub_ps(a, b, c);
#else
    return _mm256_sub_ps(_mm256_mul_ps(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FNMAddf(__m256 a, __m256 b, __m256 c) {
#if defined(RSIMD_FMA)
    return _mm256_fnmadd_ps(a, b, c);
#else
    return _mm256_sub_ps(c, _mm256_mul_ps(a, b));
#endif
}
RSIMD_FORCEINLINE auto FNMSubf(__m256 a, __m256 b, __m256 c) {
#if defined(RSIMD_FMA)
    return _mm256_fnmsub_ps(a, b, c);
#else
    return _mm256_sub_ps(_mm256_setzero_ps(), _mm256_add_ps(_mm256_mul_ps(a, b), c));
#endif
}

RSIMD_FORCEINLINE auto CmpEQf(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_EQ_OQ); }
RSIMD_FORCEINLINE auto CmpNEf(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_NEQ_OQ); }
RSIMD_FORCEINLINE auto CmpLTf(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_LT_OQ); }
RSIMD_FORCEINLINE auto CmpLEf(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_LE_OQ); }
RSIMD_FORCEINLINE auto CmpGTf(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_GT_OQ); }
RSIMD_FORCEINLINE auto CmpGEf(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_GE_OQ); }

RSIMD_FORCEINLINE auto Andf(__m256 a, __m256 b) { return _mm256_and_ps(a, b); }
RSIMD_FORCEINLINE auto Orf(__m256 a, __m256 b) { return _mm256_or_ps(a, b); }
RSIMD_FORCEINLINE auto Xorf(__m256 a, __m256 b) { return _mm256_xor_ps(a, b); }
RSIMD_FORCEINLINE auto AndNotf(__m256 a, __m256 b) { return _mm256_andnot_ps(a, b); }

RSIMD_FORCEINLINE auto Blendf(__m256 a, __m256 b, __m256 mask) { return _mm256_blendv_ps(a, b, mask); }
RSIMD_FORCEINLINE auto Selectf(__m256 mask, __m256 a, __m256 b) { return _mm256_blendv_ps(b, a, mask); }

RSIMD_FORCEINLINE auto Sqrtf(__m256 a) { return _mm256_sqrt_ps(a); }
RSIMD_FORCEINLINE auto RSqrtf(__m256 a) { return _mm256_rsqrt_ps(a); }
RSIMD_FORCEINLINE auto Rcpf(__m256 a) { return _mm256_rcp_ps(a); }
RSIMD_FORCEINLINE auto Minf(__m256 a, __m256 b) { return _mm256_min_ps(a, b); }
RSIMD_FORCEINLINE auto Maxf(__m256 a, __m256 b) { return _mm256_max_ps(a, b); }
RSIMD_FORCEINLINE auto Floorf(__m256 a) { return _mm256_round_ps(a, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC); }
RSIMD_FORCEINLINE auto Ceilf(__m256 a) { return _mm256_round_ps(a, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC); }
RSIMD_FORCEINLINE auto Roundf(__m256 a) { return _mm256_round_ps(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC); }

RSIMD_FORCEINLINE float HAddf(__m256 vec) {
    __m128 vlow = _mm256_castps256_ps128(vec);
    __m128 vhigh = _mm256_extractf128_ps(vec, 1);
    vlow = _mm_add_ps(vlow, vhigh);
    __m128 shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(2, 3, 0, 1));
    vlow = _mm_add_ps(vlow, shuf);
    shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(1, 0, 3, 2));
    vlow = _mm_add_ps(vlow, shuf);
    return _mm_cvtss_f32(vlow);
}
RSIMD_FORCEINLINE float HMinf(__m256 vec) {
    __m128 vlow = _mm256_castps256_ps128(vec);
    __m128 vhigh = _mm256_extractf128_ps(vec, 1);
    vlow = _mm_min_ps(vlow, vhigh);
    __m128 shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(2, 3, 0, 1));
    vlow = _mm_min_ps(vlow, shuf);
    shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(1, 0, 3, 2));
    vlow = _mm_min_ps(vlow, shuf);
    return _mm_cvtss_f32(vlow);
}
RSIMD_FORCEINLINE float HMaxf(__m256 vec) {
    __m128 vlow = _mm256_castps256_ps128(vec);
    __m128 vhigh = _mm256_extractf128_ps(vec, 1);
    vlow = _mm_max_ps(vlow, vhigh);
    __m128 shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(2, 3, 0, 1));
    vlow = _mm_max_ps(vlow, shuf);
    shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(1, 0, 3, 2));
    vlow = _mm_max_ps(vlow, shuf);
    return _mm_cvtss_f32(vlow);
}

RSIMD_FORCEINLINE auto ConvertToFloatf(__m256i a) { return _mm256_cvtepi32_ps(a); }
RSIMD_FORCEINLINE auto ConvertToIntf(__m256 a) { return _mm256_cvtps_epi32(a); }

RSIMD_FORCEINLINE auto CvtPs2Pd(__m128 a) { return _mm256_cvtps2_pd(a); }
RSIMD_FORCEINLINE auto CvtPd2Ps(__m256d a) { return _mm256_cvtpd2_ps(a); }

#elif defined(RSIMD_SSE2)

RSIMD_FORCEINLINE auto Loadf(const float* ptr) { return _mm_load_ps(ptr); }
RSIMD_FORCEINLINE auto LoadUf(const float* ptr) { return _mm_loadu_ps(ptr); }
RSIMD_FORCEINLINE void Storef(float* ptr, __m128 vec) { _mm_store_ps(ptr, vec); }
RSIMD_FORCEINLINE void StoreUf(float* ptr, __m128 vec) { _mm_storeu_ps(ptr, vec); }
RSIMD_FORCEINLINE auto Set1f(float val) { return _mm_set1_ps(val); }
RSIMD_FORCEINLINE auto Zerof() { return _mm_setzero_ps(); }
RSIMD_FORCEINLINE auto Setf(float v0, float v1, float v2, float v3) {
    return _mm_set_ps(v3, v2, v1, v0);
}

RSIMD_FORCEINLINE auto Addf(__m128 a, __m128 b) { return _mm_add_ps(a, b); }
RSIMD_FORCEINLINE auto Subf(__m128 a, __m128 b) { return _mm_sub_ps(a, b); }
RSIMD_FORCEINLINE auto Mulf(__m128 a, __m128 b) { return _mm_mul_ps(a, b); }
RSIMD_FORCEINLINE auto Divf(__m128 a, __m128 b) { return _mm_div_ps(a, b); }
RSIMD_FORCEINLINE auto Negf(__m128 a) { return _mm_sub_ps(_mm_setzero_ps(), a); }
RSIMD_FORCEINLINE auto Absf(__m128 a) {
    return _mm_and_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)));
}

RSIMD_FORCEINLINE auto FMAddf(__m128 a, __m128 b, __m128 c) {
#if defined(RSIMD_FMA)
    return _mm_fmadd_ps(a, b, c);
#else
    return _mm_add_ps(_mm_mul_ps(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FMSubf(__m128 a, __m128 b, __m128 c) {
#if defined(RSIMD_FMA)
    return _mm_fmsub_ps(a, b, c);
#else
    return _mm_sub_ps(_mm_mul_ps(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FNMAddf(__m128 a, __m128 b, __m128 c) {
#if defined(RSIMD_FMA)
    return _mm_fnmadd_ps(a, b, c);
#else
    return _mm_sub_ps(c, _mm_mul_ps(a, b));
#endif
}
RSIMD_FORCEINLINE auto FNMSubf(__m128 a, __m128 b, __m128 c) {
#if defined(RSIMD_FMA)
    return _mm_fnmsub_ps(a, b, c);
#else
    return _mm_sub_ps(_mm_setzero_ps(), _mm_add_ps(_mm_mul_ps(a, b), c));
#endif
}

RSIMD_FORCEINLINE auto CmpEQf(__m128 a, __m128 b) { return _mm_cmpeq_ps(a, b); }
RSIMD_FORCEINLINE auto CmpNEf(__m128 a, __m128 b) { return _mm_cmpneq_ps(a, b); }
RSIMD_FORCEINLINE auto CmpLTf(__m128 a, __m128 b) { return _mm_cmplt_ps(a, b); }
RSIMD_FORCEINLINE auto CmpLEf(__m128 a, __m128 b) { return _mm_cmple_ps(a, b); }
RSIMD_FORCEINLINE auto CmpGTf(__m128 a, __m128 b) { return _mm_cmpgt_ps(a, b); }
RSIMD_FORCEINLINE auto CmpGEf(__m128 a, __m128 b) { return _mm_cmpge_ps(a, b); }

RSIMD_FORCEINLINE auto Andf(__m128 a, __m128 b) { return _mm_and_ps(a, b); }
RSIMD_FORCEINLINE auto Orf(__m128 a, __m128 b) { return _mm_or_ps(a, b); }
RSIMD_FORCEINLINE auto Xorf(__m128 a, __m128 b) { return _mm_xor_ps(a, b); }
RSIMD_FORCEINLINE auto AndNotf(__m128 a, __m128 b) { return _mm_andnot_ps(a, b); }

#if defined(RSIMD_SSE42)
RSIMD_FORCEINLINE auto Blendf(__m128 a, __m128 b, __m128 mask) { return _mm_blendv_ps(a, b, mask); }
RSIMD_FORCEINLINE auto Selectf(__m128 mask, __m128 a, __m128 b) { return _mm_blendv_ps(b, a, mask); }
#else
RSIMD_FORCEINLINE auto Blendf(__m128 a, __m128 b, __m128 mask) {
    return _mm_or_ps(_mm_and_ps(mask, b), _mm_andnot_ps(mask, a));
}
RSIMD_FORCEINLINE auto Selectf(__m128 mask, __m128 a, __m128 b) {
    return _mm_or_ps(_mm_and_ps(mask, a), _mm_andnot_ps(mask, b));
}
#endif

RSIMD_FORCEINLINE auto Sqrtf(__m128 a) { return _mm_sqrt_ps(a); }
RSIMD_FORCEINLINE auto RSqrtf(__m128 a) { return _mm_rsqrt_ps(a); }
RSIMD_FORCEINLINE auto Rcpf(__m128 a) { return _mm_rcp_ps(a); }
RSIMD_FORCEINLINE auto Minf(__m128 a, __m128 b) { return _mm_min_ps(a, b); }
RSIMD_FORCEINLINE auto Maxf(__m128 a, __m128 b) { return _mm_max_ps(a, b); }
#if defined(RSIMD_SSE42)
RSIMD_FORCEINLINE auto Floorf(__m128 a) { return _mm_floor_ps(a); }
RSIMD_FORCEINLINE auto Ceilf(__m128 a) { return _mm_ceil_ps(a); }
RSIMD_FORCEINLINE auto Roundf(__m128 a) { return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC); }
#else
RSIMD_FORCEINLINE auto Floorf(__m128 a) {
    __m128i ai = _mm_cvttps_epi32(a);
    __m128 af = _mm_cvtepi32_ps(ai);
    __m128 mask = _mm_cmpgt_ps(af, a);
    return _mm_cvtepi32_ps(_mm_sub_epi32(ai, _mm_castps_si128(_mm_and_ps(mask, _mm_set1_ps(1.0f)))));
}
RSIMD_FORCEINLINE auto Ceilf(__m128 a) {
    __m128i ai = _mm_cvttps_epi32(a);
    __m128 af = _mm_cvtepi32_ps(ai);
    __m128 mask = _mm_cmplt_ps(af, a);
    return _mm_cvtepi32_ps(_mm_add_epi32(ai, _mm_castps_si128(_mm_and_ps(mask, _mm_set1_ps(1.0f)))));
}
RSIMD_FORCEINLINE auto Roundf(__m128 a) {
    return _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
}
#endif

RSIMD_FORCEINLINE float HAddf(__m128 vec) {
    __m128 shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 3, 0, 1));
    vec = _mm_add_ps(vec, shuf);
    shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 0, 3, 2));
    vec = _mm_add_ps(vec, shuf);
    return _mm_cvtss_f32(vec);
}
RSIMD_FORCEINLINE float HMinf(__m128 vec) {
    __m128 shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 3, 0, 1));
    vec = _mm_min_ps(vec, shuf);
    shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 0, 3, 2));
    vec = _mm_min_ps(vec, shuf);
    return _mm_cvtss_f32(vec);
}
RSIMD_FORCEINLINE float HMaxf(__m128 vec) {
    __m128 shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 3, 0, 1));
    vec = _mm_max_ps(vec, shuf);
    shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 0, 3, 2));
    vec = _mm_max_ps(vec, shuf);
    return _mm_cvtss_f32(vec);
}

RSIMD_FORCEINLINE auto ConvertToFloatf(__m128i a) { return _mm_cvtepi32_ps(a); }
RSIMD_FORCEINLINE auto ConvertToIntf(__m128 a) { return _mm_cvtps_epi32(a); }

RSIMD_FORCEINLINE auto CvtPs2Pd(__m128 a) { return _mm_cvtps_pd(a); }
RSIMD_FORCEINLINE auto CvtPd2Ps(__m128d a) { return _mm_cvtpd_ps(a); }

#endif

// =========================================================================
// double 实现
// =========================================================================

#if defined(RSIMD_AVX)

RSIMD_FORCEINLINE auto Loadd(const double* ptr) { return _mm256_load_pd(ptr); }
RSIMD_FORCEINLINE auto LoadUd(const double* ptr) { return _mm256_loadu_pd(ptr); }
RSIMD_FORCEINLINE void Stored(double* ptr, __m256d vec) { _mm256_store_pd(ptr, vec); }
RSIMD_FORCEINLINE void StoreUd(double* ptr, __m256d vec) { _mm256_storeu_pd(ptr, vec); }
RSIMD_FORCEINLINE auto Set1d(double val) { return _mm256_set1_pd(val); }
RSIMD_FORCEINLINE auto Zerod() { return _mm256_setzero_pd(); }
RSIMD_FORCEINLINE auto Setd(double v0, double v1, double v2, double v3) {
    return _mm256_set_pd(v3, v2, v1, v0);
}

RSIMD_FORCEINLINE auto Addd(__m256d a, __m256d b) { return _mm256_add_pd(a, b); }
RSIMD_FORCEINLINE auto Subd(__m256d a, __m256d b) { return _mm256_sub_pd(a, b); }
RSIMD_FORCEINLINE auto Muld(__m256d a, __m256d b) { return _mm256_mul_pd(a, b); }
RSIMD_FORCEINLINE auto Divd(__m256d a, __m256d b) { return _mm256_div_pd(a, b); }
RSIMD_FORCEINLINE auto Negd(__m256d a) { return _mm256_sub_pd(_mm256_setzero_pd(), a); }
RSIMD_FORCEINLINE auto Absd(__m256d a) {
    return _mm256_and_pd(a, _mm256_castsi256_pd(_mm256_set1_epi64x(0x7FFFFFFFFFFFFFFFLL)));
}

RSIMD_FORCEINLINE auto FMAddd(__m256d a, __m256d b, __m256d c) {
#if defined(RSIMD_FMA)
    return _mm256_fmadd_pd(a, b, c);
#else
    return _mm256_add_pd(_mm256_mul_pd(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FMSubd(__m256d a, __m256d b, __m256d c) {
#if defined(RSIMD_FMA)
    return _mm256_fmsub_pd(a, b, c);
#else
    return _mm256_sub_pd(_mm256_mul_pd(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FNMAddd(__m256d a, __m256d b, __m256d c) {
#if defined(RSIMD_FMA)
    return _mm256_fnmadd_pd(a, b, c);
#else
    return _mm256_sub_pd(c, _mm256_mul_pd(a, b));
#endif
}
RSIMD_FORCEINLINE auto FNMSubd(__m256d a, __m256d b, __m256d c) {
#if defined(RSIMD_FMA)
    return _mm256_fnmsub_pd(a, b, c);
#else
    return _mm256_sub_pd(_mm256_setzero_pd(), _mm256_add_pd(_mm256_mul_pd(a, b), c));
#endif
}

RSIMD_FORCEINLINE auto CmpEQd(__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_EQ_OQ); }
RSIMD_FORCEINLINE auto CmpNEd(__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_NEQ_OQ); }
RSIMD_FORCEINLINE auto CmpLTd(__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_LT_OQ); }
RSIMD_FORCEINLINE auto CmpLEd(__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_LE_OQ); }
RSIMD_FORCEINLINE auto CmpGTd(__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_GT_OQ); }
RSIMD_FORCEINLINE auto CmpGEd(__m256d a, __m256d b) { return _mm256_cmp_pd(a, b, _CMP_GE_OQ); }

RSIMD_FORCEINLINE auto Andd(__m256d a, __m256d b) { return _mm256_and_pd(a, b); }
RSIMD_FORCEINLINE auto Ord(__m256d a, __m256d b) { return _mm256_or_pd(a, b); }
RSIMD_FORCEINLINE auto Xord(__m256d a, __m256d b) { return _mm256_xor_pd(a, b); }
RSIMD_FORCEINLINE auto AndNotd(__m256d a, __m256d b) { return _mm256_andnot_pd(a, b); }

RSIMD_FORCEINLINE auto Blendd(__m256d a, __m256d b, __m256d mask) { return _mm256_blendv_pd(a, b, mask); }
RSIMD_FORCEINLINE auto Selectd(__m256d mask, __m256d a, __m256d b) { return _mm256_blendv_pd(b, a, mask); }

RSIMD_FORCEINLINE auto Sqrtd(__m256d a) { return _mm256_sqrt_pd(a); }
RSIMD_FORCEINLINE auto Mind(__m256d a, __m256d b) { return _mm256_min_pd(a, b); }
RSIMD_FORCEINLINE auto Maxd(__m256d a, __m256d b) { return _mm256_max_pd(a, b); }
RSIMD_FORCEINLINE auto Floord(__m256d a) { return _mm256_round_pd(a, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC); }
RSIMD_FORCEINLINE auto Ceild(__m256d a) { return _mm256_round_pd(a, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC); }
RSIMD_FORCEINLINE auto Roundd(__m256d a) { return _mm256_round_pd(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC); }

RSIMD_FORCEINLINE double HAddd(__m256d vec) {
    __m128d vlow = _mm256_castpd256_pd128(vec);
    __m128d vhigh = _mm256_extractf128_pd(vec, 1);
    vlow = _mm_add_pd(vlow, vhigh);
    __m128d shuf = _mm_shuffle_pd(vlow, vlow, _MM_SHUFFLE2(0, 1));
    vlow = _mm_add_pd(vlow, shuf);
    return _mm_cvtsd_f64(vlow);
}
RSIMD_FORCEINLINE double HMind(__m256d vec) {
    __m128d vlow = _mm256_castpd256_pd128(vec);
    __m128d vhigh = _mm256_extractf128_pd(vec, 1);
    vlow = _mm_min_pd(vlow, vhigh);
    __m128d shuf = _mm_shuffle_pd(vlow, vlow, _MM_SHUFFLE2(0, 1));
    vlow = _mm_min_pd(vlow, shuf);
    return _mm_cvtsd_f64(vlow);
}
RSIMD_FORCEINLINE double HMaxd(__m256d vec) {
    __m128d vlow = _mm256_castpd256_pd128(vec);
    __m128d vhigh = _mm256_extractf128_pd(vec, 1);
    vlow = _mm_max_pd(vlow, vhigh);
    __m128d shuf = _mm_shuffle_pd(vlow, vlow, _MM_SHUFFLE2(0, 1));
    vlow = _mm_max_pd(vlow, shuf);
    return _mm_cvtsd_f64(vlow);
}

RSIMD_FORCEINLINE auto ConvertToFloatd(__m128i a) { return _mm_cvtepi32_pd(a); }
RSIMD_FORCEINLINE auto ConvertToIntd(__m256d a) { return _mm256_cvtpd_epi32(a); }

#elif defined(RSIMD_SSE2)

RSIMD_FORCEINLINE auto Loadd(const double* ptr) { return _mm_load_pd(ptr); }
RSIMD_FORCEINLINE auto LoadUd(const double* ptr) { return _mm_loadu_pd(ptr); }
RSIMD_FORCEINLINE void Stored(double* ptr, __m128d vec) { _mm_store_pd(ptr, vec); }
RSIMD_FORCEINLINE void StoreUd(double* ptr, __m128d vec) { _mm_storeu_pd(ptr, vec); }
RSIMD_FORCEINLINE auto Set1d(double val) { return _mm_set1_pd(val); }
RSIMD_FORCEINLINE auto Zerod() { return _mm_setzero_pd(); }
RSIMD_FORCEINLINE auto Setd(double v0, double v1) {
    return _mm_set_pd(v1, v0);
}

RSIMD_FORCEINLINE auto Addd(__m128d a, __m128d b) { return _mm_add_pd(a, b); }
RSIMD_FORCEINLINE auto Subd(__m128d a, __m128d b) { return _mm_sub_pd(a, b); }
RSIMD_FORCEINLINE auto Muld(__m128d a, __m128d b) { return _mm_mul_pd(a, b); }
RSIMD_FORCEINLINE auto Divd(__m128d a, __m128d b) { return _mm_div_pd(a, b); }
RSIMD_FORCEINLINE auto Negd(__m128d a) { return _mm_sub_pd(_mm_setzero_pd(), a); }
RSIMD_FORCEINLINE auto Absd(__m128d a) {
    return _mm_and_pd(a, _mm_castsi128_pd(_mm_set1_epi64x(0x7FFFFFFFFFFFFFFFLL)));
}

RSIMD_FORCEINLINE auto FMAddd(__m128d a, __m128d b, __m128d c) {
#if defined(RSIMD_FMA)
    return _mm_fmadd_pd(a, b, c);
#else
    return _mm_add_pd(_mm_mul_pd(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FMSubd(__m128d a, __m128d b, __m128d c) {
#if defined(RSIMD_FMA)
    return _mm_fmsub_pd(a, b, c);
#else
    return _mm_sub_pd(_mm_mul_pd(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FNMAddd(__m128d a, __m128d b, __m128d c) {
#if defined(RSIMD_FMA)
    return _mm_fnmadd_pd(a, b, c);
#else
    return _mm_sub_pd(c, _mm_mul_pd(a, b));
#endif
}
RSIMD_FORCEINLINE auto FNMSubd(__m128d a, __m128d b, __m128d c) {
#if defined(RSIMD_FMA)
    return _mm_fnmsub_pd(a, b, c);
#else
    return _mm_sub_pd(_mm_setzero_pd(), _mm_add_pd(_mm_mul_pd(a, b), c));
#endif
}

RSIMD_FORCEINLINE auto CmpEQd(__m128d a, __m128d b) { return _mm_cmpeq_pd(a, b); }
RSIMD_FORCEINLINE auto CmpNEd(__m128d a, __m128d b) { return _mm_cmpneq_pd(a, b); }
RSIMD_FORCEINLINE auto CmpLTd(__m128d a, __m128d b) { return _mm_cmplt_pd(a, b); }
RSIMD_FORCEINLINE auto CmpLEd(__m128d a, __m128d b) { return _mm_cmple_pd(a, b); }
RSIMD_FORCEINLINE auto CmpGTd(__m128d a, __m128d b) { return _mm_cmpgt_pd(a, b); }
RSIMD_FORCEINLINE auto CmpGEd(__m128d a, __m128d b) { return _mm_cmpge_pd(a, b); }

RSIMD_FORCEINLINE auto Andd(__m128d a, __m128d b) { return _mm_and_pd(a, b); }
RSIMD_FORCEINLINE auto Ord(__m128d a, __m128d b) { return _mm_or_pd(a, b); }
RSIMD_FORCEINLINE auto Xord(__m128d a, __m128d b) { return _mm_xor_pd(a, b); }
RSIMD_FORCEINLINE auto AndNotd(__m128d a, __m128d b) { return _mm_andnot_pd(a, b); }

#if defined(RSIMD_SSE42)
RSIMD_FORCEINLINE auto Blendd(__m128d a, __m128d b, __m128d mask) { return _mm_blendv_pd(a, b, mask); }
RSIMD_FORCEINLINE auto Selectd(__m128d mask, __m128d a, __m128d b) { return _mm_blendv_pd(b, a, mask); }
#else
RSIMD_FORCEINLINE auto Blendd(__m128d a, __m128d b, __m128d mask) {
    return _mm_or_pd(_mm_and_pd(mask, b), _mm_andnot_pd(mask, a));
}
RSIMD_FORCEINLINE auto Selectd(__m128d mask, __m128d a, __m128d b) {
    return _mm_or_pd(_mm_and_pd(mask, a), _mm_andnot_pd(mask, b));
}
#endif

RSIMD_FORCEINLINE auto Sqrtd(__m128d a) { return _mm_sqrt_pd(a); }
RSIMD_FORCEINLINE auto Mind(__m128d a, __m128d b) { return _mm_min_pd(a, b); }
RSIMD_FORCEINLINE auto Maxd(__m128d a, __m128d b) { return _mm_max_pd(a, b); }
#if defined(RSIMD_SSE42)
RSIMD_FORCEINLINE auto Floord(__m128d a) { return _mm_floor_pd(a); }
RSIMD_FORCEINLINE auto Ceild(__m128d a) { return _mm_ceil_pd(a); }
RSIMD_FORCEINLINE auto Roundd(__m128d a) { return _mm_round_pd(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC); }
#else
RSIMD_FORCEINLINE auto Floord(__m128d a) {
    __m128i ai = _mm_cvttpd_epi32(a);
    __m128d af = _mm_cvtepi32_pd(ai);
    __m128d mask = _mm_cmpgt_pd(af, a);
    return _mm_cvtepi32_pd(_mm_sub_epi32(ai, _mm_castpd_si128(_mm_and_pd(mask, _mm_castsi128_pd(_mm_set1_epi32(1))))));
}
RSIMD_FORCEINLINE auto Ceild(__m128d a) {
    __m128i ai = _mm_cvttpd_epi32(a);
    __m128d af = _mm_cvtepi32_pd(ai);
    __m128d mask = _mm_cmplt_pd(af, a);
    return _mm_cvtepi32_pd(_mm_add_epi32(ai, _mm_castpd_si128(_mm_and_pd(mask, _mm_castsi128_pd(_mm_set1_epi32(1))))));
}
RSIMD_FORCEINLINE auto Roundd(__m128d a) {
    return _mm_cvtepi32_pd(_mm_cvtpd_epi32(a));
}
#endif

RSIMD_FORCEINLINE double HAddd(__m128d vec) {
    __m128d shuf = _mm_shuffle_pd(vec, vec, _MM_SHUFFLE2(0, 1));
    vec = _mm_add_pd(vec, shuf);
    return _mm_cvtsd_f64(vec);
}
RSIMD_FORCEINLINE double HMind(__m128d vec) {
    __m128d shuf = _mm_shuffle_pd(vec, vec, _MM_SHUFFLE2(0, 1));
    vec = _mm_min_pd(vec, shuf);
    return _mm_cvtsd_f64(vec);
}
RSIMD_FORCEINLINE double HMaxd(__m128d vec) {
    __m128d shuf = _mm_shuffle_pd(vec, vec, _MM_SHUFFLE2(0, 1));
    vec = _mm_max_pd(vec, shuf);
    return _mm_cvtsd_f64(vec);
}

RSIMD_FORCEINLINE auto ConvertToFloatd(__m128i a) { return _mm_cvtepi32_pd(a); }
RSIMD_FORCEINLINE auto ConvertToIntd(__m128d a) { return _mm_cvtpd_epi32(a); }

#endif

// =========================================================================
// int32_t 实现
// =========================================================================

#if defined(RSIMD_AVX)

RSIMD_FORCEINLINE auto Loadi(const int32_t* ptr) {
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
}
RSIMD_FORCEINLINE auto LoadUi(const int32_t* ptr) {
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
RSIMD_FORCEINLINE void Storei(int32_t* ptr, __m256i vec) {
    _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), vec);
}
RSIMD_FORCEINLINE void StoreUi(int32_t* ptr, __m256i vec) {
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), vec);
}
RSIMD_FORCEINLINE auto Set1i(int32_t val) { return _mm256_set1_epi32(val); }
RSIMD_FORCEINLINE auto Zeroi() { return _mm256_setzero_si256(); }
RSIMD_FORCEINLINE auto Seti(int32_t v0, int32_t v1, int32_t v2, int32_t v3, int32_t v4, int32_t v5, int32_t v6, int32_t v7) {
    return _mm256_set_epi32(v7, v6, v5, v4, v3, v2, v1, v0);
}

RSIMD_FORCEINLINE auto Addi(__m256i a, __m256i b) { return _mm256_add_epi32(a, b); }
RSIMD_FORCEINLINE auto Subi(__m256i a, __m256i b) { return _mm256_sub_epi32(a, b); }
RSIMD_FORCEINLINE auto Muli(__m256i a, __m256i b) {
#if defined(RSIMD_AVX2)
    return _mm256_mullo_epi32(a, b);
#else
    __m128i a_lo = _mm256_castsi256_si128(a);
    __m128i a_hi = _mm256_extractf128_si256(a, 1);
    __m128i b_lo = _mm256_castsi256_si128(b);
    __m128i b_hi = _mm256_extractf128_si256(b, 1);
    a_lo = _mm_mullo_epi32(a_lo, b_lo);
    a_hi = _mm_mullo_epi32(a_hi, b_hi);
    __m256i result = _mm256_castsi128_si256(a_lo);
    return _mm256_insertf128_si256(result, a_hi, 1);
#endif
}
RSIMD_FORCEINLINE auto Negi(__m256i a) { return _mm256_sub_epi32(_mm256_setzero_si256(), a); }
RSIMD_FORCEINLINE auto Absi(__m256i a) {
#if defined(RSIMD_AVX2)
    return _mm256_abs_epi32(a);
#else
    __m128i lo = _mm256_castsi256_si128(a);
    __m128i hi = _mm256_extractf128_si256(a, 1);
    __m128i lo_mask = _mm_srai_epi32(lo, 31);
    __m128i hi_mask = _mm_srai_epi32(hi, 31);
    lo = _mm_sub_epi32(_mm_xor_si128(lo, lo_mask), lo_mask);
    hi = _mm_sub_epi32(_mm_xor_si128(hi, hi_mask), hi_mask);
    __m256i result = _mm256_castsi128_si256(lo);
    return _mm256_insertf128_si256(result, hi, 1);
#endif
}

RSIMD_FORCEINLINE auto CmpEQi(__m256i a, __m256i b) { return _mm256_cmpeq_epi32(a, b); }
RSIMD_FORCEINLINE auto CmpLTi(__m256i a, __m256i b) { return _mm256_cmpgt_epi32(b, a); }
RSIMD_FORCEINLINE auto CmpGTi(__m256i a, __m256i b) { return _mm256_cmpgt_epi32(a, b); }

RSIMD_FORCEINLINE auto Andi(__m256i a, __m256i b) { return _mm256_and_si256(a, b); }
RSIMD_FORCEINLINE auto Ori(__m256i a, __m256i b) { return _mm256_or_si256(a, b); }
RSIMD_FORCEINLINE auto Xori(__m256i a, __m256i b) { return _mm256_xor_si256(a, b); }
RSIMD_FORCEINLINE auto AndNoti(__m256i a, __m256i b) { return _mm256_andnot_si256(a, b); }

RSIMD_FORCEINLINE auto Blendvi(__m256i a, __m256i b, __m256i mask) {
#if defined(RSIMD_AVX2)
    return _mm256_blendv_epi8(a, b, mask);
#else
    return _mm256_or_si256(_mm256_and_si256(mask, b), _mm256_andnot_si256(mask, a));
#endif
}
RSIMD_FORCEINLINE auto Selecti(__m256i mask, __m256i a, __m256i b) {
    return Blendvi(b, a, mask);
}

RSIMD_FORCEINLINE auto Mini(__m256i a, __m256i b) {
#if defined(RSIMD_AVX2)
    return _mm256_min_epi32(a, b);
#else
    __m256i mask = _mm256_cmpgt_epi32(a, b);
    return Blendvi(a, b, mask);
#endif
}
RSIMD_FORCEINLINE auto Maxi(__m256i a, __m256i b) {
#if defined(RSIMD_AVX2)
    return _mm256_max_epi32(a, b);
#else
    __m256i mask = _mm256_cmpgt_epi32(a, b);
    return Blendvi(b, a, mask);
#endif
}

RSIMD_FORCEINLINE int32_t HAddi(__m256i vec) {
    __m128i vlow = _mm256_castsi256_si128(vec);
    __m128i vhigh = _mm256_extractf128_si256(vec, 1);
    vlow = _mm_add_epi32(vlow, vhigh);
    __m128i hi32 = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(2, 3, 0, 1));
    vlow = _mm_add_epi32(vlow, hi32);
    __m128i hi16 = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(1, 0, 3, 2));
    vlow = _mm_add_epi32(vlow, hi16);
    return _mm_cvtsi128_si32(vlow);
}
RSIMD_FORCEINLINE int32_t HMini(__m256i vec) {
    __m128i vlow = _mm256_castsi256_si128(vec);
    __m128i vhigh = _mm256_extractf128_si256(vec, 1);
    vlow = _mm_min_epi32(vlow, vhigh);
    __m128i hi32 = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(2, 3, 0, 1));
    vlow = _mm_min_epi32(vlow, hi32);
    __m128i hi16 = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(1, 0, 3, 2));
    vlow = _mm_min_epi32(vlow, hi16);
    return _mm_cvtsi128_si32(vlow);
}
RSIMD_FORCEINLINE int32_t HMaxi(__m256i vec) {
    __m128i vlow = _mm256_castsi256_si128(vec);
    __m128i vhigh = _mm256_extractf128_si256(vec, 1);
    vlow = _mm_max_epi32(vlow, vhigh);
    __m128i hi32 = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(2, 3, 0, 1));
    vlow = _mm_max_epi32(vlow, hi32);
    __m128i hi16 = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(1, 0, 3, 2));
    vlow = _mm_max_epi32(vlow, hi16);
    return _mm_cvtsi128_si32(vlow);
}

RSIMD_FORCEINLINE auto ConvertToFloati(__m256i a) { return _mm256_cvtepi32_ps(a); }
RSIMD_FORCEINLINE auto ConvertToInti(__m256 a) { return _mm256_cvtps_epi32(a); }

#elif defined(RSIMD_SSE2)

RSIMD_FORCEINLINE auto Loadi(const int32_t* ptr) {
    return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
}
RSIMD_FORCEINLINE auto LoadUi(const int32_t* ptr) {
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
RSIMD_FORCEINLINE void Storei(int32_t* ptr, __m128i vec) {
    _mm_store_si128(reinterpret_cast<__m128i*>(ptr), vec);
}
RSIMD_FORCEINLINE void StoreUi(int32_t* ptr, __m128i vec) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), vec);
}
RSIMD_FORCEINLINE auto Set1i(int32_t val) { return _mm_set1_epi32(val); }
RSIMD_FORCEINLINE auto Zeroi() { return _mm_setzero_si128(); }
RSIMD_FORCEINLINE auto Seti(int32_t v0, int32_t v1, int32_t v2, int32_t v3) {
    return _mm_set_epi32(v3, v2, v1, v0);
}

RSIMD_FORCEINLINE auto Addi(__m128i a, __m128i b) { return _mm_add_epi32(a, b); }
RSIMD_FORCEINLINE auto Subi(__m128i a, __m128i b) { return _mm_sub_epi32(a, b); }
RSIMD_FORCEINLINE auto Muli(__m128i a, __m128i b) {
#if defined(RSIMD_SSE42)
    return _mm_mullo_epi32(a, b);
#else
    __m128i tmp1 = _mm_mul_epu32(a, b);
    __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
    return _mm_unpacklo_epi32(
        _mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)),
        _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
#endif
}
RSIMD_FORCEINLINE auto Negi(__m128i a) { return _mm_sub_epi32(_mm_setzero_si128(), a); }
RSIMD_FORCEINLINE auto Absi(__m128i a) {
#if defined(RSIMD_SSE3)
    return _mm_abs_epi32(a);
#else
    __m128i mask = _mm_srai_epi32(a, 31);
    return _mm_sub_epi32(_mm_xor_si128(a, mask), mask);
#endif
}

RSIMD_FORCEINLINE auto CmpEQi(__m128i a, __m128i b) { return _mm_cmpeq_epi32(a, b); }
RSIMD_FORCEINLINE auto CmpLTi(__m128i a, __m128i b) { return _mm_cmplt_epi32(a, b); }
RSIMD_FORCEINLINE auto CmpGTi(__m128i a, __m128i b) { return _mm_cmpgt_epi32(a, b); }

RSIMD_FORCEINLINE auto Andi(__m128i a, __m128i b) { return _mm_and_si128(a, b); }
RSIMD_FORCEINLINE auto Ori(__m128i a, __m128i b) { return _mm_or_si128(a, b); }
RSIMD_FORCEINLINE auto Xori(__m128i a, __m128i b) { return _mm_xor_si128(a, b); }
RSIMD_FORCEINLINE auto AndNoti(__m128i a, __m128i b) { return _mm_andnot_si128(a, b); }

RSIMD_FORCEINLINE auto Blendvi(__m128i a, __m128i b, __m128i mask) {
    return _mm_or_si128(_mm_and_si128(mask, b), _mm_andnot_si128(mask, a));
}
RSIMD_FORCEINLINE auto Selecti(__m128i mask, __m128i a, __m128i b) {
    return Blendvi(b, a, mask);
}

RSIMD_FORCEINLINE auto Mini(__m128i a, __m128i b) {
#if defined(RSIMD_SSE42)
    return _mm_min_epi32(a, b);
#else
    __m128i mask = _mm_cmpgt_epi32(a, b);
    return Blendvi(a, b, mask);
#endif
}
RSIMD_FORCEINLINE auto Maxi(__m128i a, __m128i b) {
#if defined(RSIMD_SSE42)
    return _mm_max_epi32(a, b);
#else
    __m128i mask = _mm_cmpgt_epi32(a, b);
    return Blendvi(b, a, mask);
#endif
}

RSIMD_FORCEINLINE int32_t HAddi(__m128i vec) {
    __m128i hi64 = _mm_unpackhi_epi64(vec, vec);
    vec = _mm_add_epi32(vec, hi64);
    __m128i hi32 = _mm_shuffle_epi32(vec, _MM_SHUFFLE(2, 3, 0, 1));
    vec = _mm_add_epi32(vec, hi32);
    return _mm_cvtsi128_si32(vec);
}
RSIMD_FORCEINLINE int32_t HMini(__m128i vec) {
    __m128i hi64 = _mm_unpackhi_epi64(vec, vec);
    vec = Mini(vec, hi64);
    __m128i hi32 = _mm_shuffle_epi32(vec, _MM_SHUFFLE(2, 3, 0, 1));
    vec = Mini(vec, hi32);
    return _mm_cvtsi128_si32(vec);
}
RSIMD_FORCEINLINE int32_t HMaxi(__m128i vec) {
    __m128i hi64 = _mm_unpackhi_epi64(vec, vec);
    vec = Maxi(vec, hi64);
    __m128i hi32 = _mm_shuffle_epi32(vec, _MM_SHUFFLE(2, 3, 0, 1));
    vec = Maxi(vec, hi32);
    return _mm_cvtsi128_si32(vec);
}

RSIMD_FORCEINLINE auto ConvertToFloati(__m128i a) { return _mm_cvtepi32_ps(a); }
RSIMD_FORCEINLINE auto ConvertToInti(__m128 a) { return _mm_cvtps_epi32(a); }

#endif

// =========================================================================
// 泛型分发层 (模板函数，根据类型自动选择 SIMD 实现)
// =========================================================================

// --- 内存操作 ---
template <typename T> RSIMD_FORCEINLINE auto Load(const T* ptr) {
    if constexpr (std::is_same_v<T, float>) return Loadf(ptr);
    else if constexpr (std::is_same_v<T, double>) return Loadd(ptr);
    else if constexpr (std::is_same_v<T, int32_t>) return Loadi(ptr);
}
template <typename T> RSIMD_FORCEINLINE auto LoadU(const T* ptr) {
    if constexpr (std::is_same_v<T, float>) return LoadUf(ptr);
    else if constexpr (std::is_same_v<T, double>) return LoadUd(ptr);
    else if constexpr (std::is_same_v<T, int32_t>) return LoadUi(ptr);
}
template <typename T> RSIMD_FORCEINLINE void Store(T* ptr, auto vec) {
    if constexpr (std::is_same_v<T, float>) Storef(ptr, vec);
    else if constexpr (std::is_same_v<T, double>) Stored(ptr, vec);
    else if constexpr (std::is_same_v<T, int32_t>) Storei(ptr, vec);
}
template <typename T> RSIMD_FORCEINLINE void StoreU(T* ptr, auto vec) {
    if constexpr (std::is_same_v<T, float>) StoreUf(ptr, vec);
    else if constexpr (std::is_same_v<T, double>) StoreUd(ptr, vec);
    else if constexpr (std::is_same_v<T, int32_t>) StoreUi(ptr, vec);
}
template <typename T> RSIMD_FORCEINLINE auto Set1(T val) {
    if constexpr (std::is_same_v<T, float>) return Set1f(val);
    else if constexpr (std::is_same_v<T, double>) return Set1d(val);
    else if constexpr (std::is_same_v<T, int32_t>) return Set1i(static_cast<int32_t>(val));
}
template <typename T> RSIMD_FORCEINLINE auto Zero() {
    if constexpr (std::is_same_v<T, float>) return Zerof();
    else if constexpr (std::is_same_v<T, double>) return Zerod();
    else if constexpr (std::is_same_v<T, int32_t>) return Zeroi();
}
template <typename T> RSIMD_FORCEINLINE auto Set(T v0, T v1, T v2, T v3) {
    if constexpr (std::is_same_v<T, float>) return Setf(v0, v1, v2, v3);
    else if constexpr (std::is_same_v<T, double>) return Setd(v0, v1);
    else if constexpr (std::is_same_v<T, int32_t>) return Seti(v0, v1, v2, v3);
}
template <typename T> RSIMD_FORCEINLINE auto Set(T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7) {
    if constexpr (std::is_same_v<T, float>) return Setf(v0, v1, v2, v3, v4, v5, v6, v7);
    else if constexpr (std::is_same_v<T, double>) return Setd(v0, v1, v2, v3);
    else if constexpr (std::is_same_v<T, int32_t>) return Seti(v0, v1, v2, v3, v4, v5, v6, v7);
}

// --- 算术运算 ---
template <typename T> RSIMD_FORCEINLINE auto Add(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Addf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Addd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Addi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto Sub(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Subf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Subd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Subi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto Mul(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Mulf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Muld(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Muli(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto Div(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Divf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Divd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return a;
}
template <typename T> RSIMD_FORCEINLINE auto Neg(auto a) {
    if constexpr (std::is_same_v<T, float>) return Negf(a);
    else if constexpr (std::is_same_v<T, double>) return Negd(a);
    else if constexpr (std::is_same_v<T, int32_t>) return Negi(a);
}
template <typename T> RSIMD_FORCEINLINE auto Abs(auto a) {
    if constexpr (std::is_same_v<T, float>) return Absf(a);
    else if constexpr (std::is_same_v<T, double>) return Absd(a);
    else if constexpr (std::is_same_v<T, int32_t>) return Absi(a);
}

// --- FMA ---
template <typename T> RSIMD_FORCEINLINE auto FMAdd(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FMAddf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return FMAddd(a, b, c);
    else if constexpr (std::is_same_v<T, int32_t>) return Addi(Muli(a, b), c);
}
template <typename T> RSIMD_FORCEINLINE auto FMSub(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FMSubf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return FMSubd(a, b, c);
    else if constexpr (std::is_same_v<T, int32_t>) return Subi(Muli(a, b), c);
}
template <typename T> RSIMD_FORCEINLINE auto FNMAdd(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FNMAddf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return FNMAddd(a, b, c);
    else if constexpr (std::is_same_v<T, int32_t>) return Subi(c, Muli(a, b));
}
template <typename T> RSIMD_FORCEINLINE auto FNMSub(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FNMSubf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return FNMSubd(a, b, c);
    else if constexpr (std::is_same_v<T, int32_t>) return Negi(Addi(Muli(a, b), c));
}

// --- 比较运算 ---
template <typename T> RSIMD_FORCEINLINE auto CmpEQ(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpEQf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpEQd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return CmpEQi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto CmpNE(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpNEf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpNEd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Xori(CmpEQi(a, b), Set1i(0xFFFFFFFF));
}
template <typename T> RSIMD_FORCEINLINE auto CmpLT(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpLTf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpLTd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return CmpLTi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto CmpLE(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpLEf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpLEd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Ori(CmpLTi(a, b), CmpEQi(a, b));
}
template <typename T> RSIMD_FORCEINLINE auto CmpGT(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpGTf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpGTd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return CmpGTi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto CmpGE(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpGEf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpGEd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Ori(CmpGTi(a, b), CmpEQi(a, b));
}

// --- 逻辑运算 ---
template <typename T> RSIMD_FORCEINLINE auto And(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Andf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Andd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Andi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto Or(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Orf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Ord(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Ori(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto Xor(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Xorf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Xord(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Xori(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto AndNot(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return AndNotf(a, b);
    else if constexpr (std::is_same_v<T, double>) return AndNotd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return AndNoti(a, b);
}

// --- 混合/选择 ---
template <typename T> RSIMD_FORCEINLINE auto Blend(auto a, auto b, auto mask) {
    if constexpr (std::is_same_v<T, float>) return Blendf(a, b, mask);
    else if constexpr (std::is_same_v<T, double>) return Blendd(a, b, mask);
    else if constexpr (std::is_same_v<T, int32_t>) return Blendvi(a, b, mask);
}
template <typename T> RSIMD_FORCEINLINE auto Select(auto mask, auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Selectf(mask, a, b);
    else if constexpr (std::is_same_v<T, double>) return Selectd(mask, a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Selecti(mask, a, b);
}

// --- 数学函数 ---
template <typename T> RSIMD_FORCEINLINE auto Sqrt(auto a) {
    if constexpr (std::is_same_v<T, float>) return Sqrtf(a);
    else if constexpr (std::is_same_v<T, double>) return Sqrtd(a);
}
template <typename T> RSIMD_FORCEINLINE auto RSqrt(auto a) {
    if constexpr (std::is_same_v<T, float>) return RSqrtf(a);
}
template <typename T> RSIMD_FORCEINLINE auto Rcp(auto a) {
    if constexpr (std::is_same_v<T, float>) return Rcpf(a);
}
template <typename T> RSIMD_FORCEINLINE auto Min(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Minf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Mind(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Mini(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto Max(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return Maxf(a, b);
    else if constexpr (std::is_same_v<T, double>) return Maxd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return Maxi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto Floor(auto a) {
    if constexpr (std::is_same_v<T, float>) return Floorf(a);
    else if constexpr (std::is_same_v<T, double>) return Floord(a);
}
template <typename T> RSIMD_FORCEINLINE auto Ceil(auto a) {
    if constexpr (std::is_same_v<T, float>) return Ceilf(a);
    else if constexpr (std::is_same_v<T, double>) return Ceild(a);
}
template <typename T> RSIMD_FORCEINLINE auto Round(auto a) {
    if constexpr (std::is_same_v<T, float>) return Roundf(a);
    else if constexpr (std::is_same_v<T, double>) return Roundd(a);
}

// --- 水平归约 ---
template <typename T> RSIMD_FORCEINLINE T HAdd(auto vec) {
    if constexpr (std::is_same_v<T, float>) return HAddf(vec);
    else if constexpr (std::is_same_v<T, double>) return HAddd(vec);
    else if constexpr (std::is_same_v<T, int32_t>) return HAddi(vec);
}
template <typename T> RSIMD_FORCEINLINE T HMin(auto vec) {
    if constexpr (std::is_same_v<T, float>) return HMinf(vec);
    else if constexpr (std::is_same_v<T, double>) return HMind(vec);
    else if constexpr (std::is_same_v<T, int32_t>) return HMini(vec);
}
template <typename T> RSIMD_FORCEINLINE T HMax(auto vec) {
    if constexpr (std::is_same_v<T, float>) return HMaxf(vec);
    else if constexpr (std::is_same_v<T, double>) return HMaxd(vec);
    else if constexpr (std::is_same_v<T, int32_t>) return HMaxi(vec);
}

// --- 类型转换 ---
template <typename T> RSIMD_FORCEINLINE auto ConvertToFloat(auto a) {
    if constexpr (std::is_same_v<T, float>) return ConvertToFloatf(a);
    else if constexpr (std::is_same_v<T, double>) return ConvertToFloatd(a);
    else if constexpr (std::is_same_v<T, int32_t>) return ConvertToFloati(a);
}
template <typename T> RSIMD_FORCEINLINE auto ConvertToInt(auto a) {
    if constexpr (std::is_same_v<T, float>) return ConvertToIntf(a);
    else if constexpr (std::is_same_v<T, double>) return ConvertToIntd(a);
    else if constexpr (std::is_same_v<T, int32_t>) return ConvertToInti(a);
}

// --- 浮点类型间转换 ---
template <typename From, typename To> RSIMD_FORCEINLINE auto Convert(auto a) {
    if constexpr (std::is_same_v<From, float> && std::is_same_v<To, double>) return CvtPs2Pd(a);
    else if constexpr (std::is_same_v<From, double> && std::is_same_v<To, float>) return CvtPd2Ps(a);
    else return a;
}

} // namespace RandomEngine::Platform::SIMD