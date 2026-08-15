#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace RandEngine::Core::Math{

// SIMD detection
#if defined(__AVX2__)
    #define RMATH_AVX2
    #define RMATH_AVX
    #define RMATH_SSE42
    #define RMATH_SSE2
#elif defined(__AVX__)
    #define RMATH_AVX
    #define RMATH_SSE42
    #define RMATH_SSE2
#elif defined(__SSE4_2__)
    #define RMATH_SSE42
    #define RMATH_SSE2
#elif defined(__SSE2__)
    #define RMATH_SSE2
#endif

#if defined(_MSC_VER)
    #include <intrin.h>
#else
    #if defined(RMATH_AVX2) || defined(RMATH_AVX)
        #include <immintrin.h>
    #elif defined(RMATH_SSE42)
        #include <nmmintrin.h>
    #elif defined(RMATH_SSE2)
        #include <emmintrin.h>
    #endif
#endif

// 强制内联：避免 __m256/__m256d/__m256i 通过 ABI 传递。
// MinGW GCC 在 -O0 下对 __m256* 返回值的栈槽只做 16 字节对齐，
// 却生成对齐的 vmovapd 存储，导致段错误。强制内联可消除该问题。
#if defined(_MSC_VER)
    #define RMATH_FORCEINLINE __forceinline
#else
    #define RMATH_FORCEINLINE inline __attribute__((always_inline))
#endif

namespace Detail::simd
{
    // Check if type supports SIMD
    template <typename T>
    inline constexpr bool SupportsSIMD = false;

    template <>
    inline constexpr bool SupportsSIMD<float> = true;

    template <>
    inline constexpr bool SupportsSIMD<double> = true;

    template <>
    inline constexpr bool SupportsSIMD<int> = true;

    // SIMD width for type
    template <typename T>
    inline constexpr size_t SIMDWidth = 1;

#if defined(RMATH_AVX)
    template <>
    inline constexpr std::size_t SIMDWidth<float> = 8;

    template <>
    inline constexpr std::size_t SIMDWidth<double> = 4;

    template <>
    inline constexpr std::size_t SIMDWidth<int> = 8;
#elif defined(RMATH_SSE2)
    template <>
    inline constexpr size_t SIMDWidth<float> = 4;

    template <>
    inline constexpr size_t SIMDWidth<double> = 2;

    template <>
    inline constexpr size_t SIMDWidth<int> = 4;
#endif

#if defined(RMATH_AVX)
    // float AVX
    RMATH_FORCEINLINE auto loaduf(const float *ptr) { return _mm256_loadu_ps(ptr); }
    RMATH_FORCEINLINE void storeuf(float *ptr, __m256 vec) { _mm256_storeu_ps(ptr, vec); }
    RMATH_FORCEINLINE auto set1f(float val) { return _mm256_set1_ps(val); }
    RMATH_FORCEINLINE auto addf(__m256 a, __m256 b) { return _mm256_add_ps(a, b); }
    RMATH_FORCEINLINE auto subf(__m256 a, __m256 b) { return _mm256_sub_ps(a, b); }
    RMATH_FORCEINLINE auto mulf(__m256 a, __m256 b) { return _mm256_mul_ps(a, b); }
    RMATH_FORCEINLINE auto divf(__m256 a, __m256 b) { return _mm256_div_ps(a, b); }
    RMATH_FORCEINLINE auto fmaddf(__m256 a, __m256 b, __m256 c) {
#if defined(RMATH_AVX2)
        return _mm256_fmadd_ps(a, b, c);
#else
        return _mm256_add_ps(_mm256_mul_ps(a, b), c);
#endif
    }
    RMATH_FORCEINLINE float haddf(__m256 vec) {
        __m128 vlow = _mm256_castps256_ps128(vec);
        __m128 vhigh = _mm256_extractf128_ps(vec, 1);
        vlow = _mm_add_ps(vlow, vhigh);
        __m128 shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(2, 3, 0, 1));
        vlow = _mm_add_ps(vlow, shuf);
        shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(1, 0, 3, 2));
        vlow = _mm_add_ps(vlow, shuf);
        return _mm_cvtss_f32(vlow);
    }
    RMATH_FORCEINLINE auto zerof() { return _mm256_setzero_ps(); }

    // double AVX
    RMATH_FORCEINLINE auto loadud(const double *ptr) { return _mm256_loadu_pd(ptr); }
    RMATH_FORCEINLINE void storeud(double *ptr, __m256d vec) { _mm256_storeu_pd(ptr, vec); }
    RMATH_FORCEINLINE auto set1d(double val) { return _mm256_set1_pd(val); }
    RMATH_FORCEINLINE auto addd(__m256d a, __m256d b) { return _mm256_add_pd(a, b); }
    RMATH_FORCEINLINE auto subd(__m256d a, __m256d b) { return _mm256_sub_pd(a, b); }
    RMATH_FORCEINLINE auto muld(__m256d a, __m256d b) { return _mm256_mul_pd(a, b); }
    RMATH_FORCEINLINE auto divd(__m256d a, __m256d b) { return _mm256_div_pd(a, b); }
    RMATH_FORCEINLINE auto fmaddd(__m256d a, __m256d b, __m256d c) {
#if defined(RMATH_AVX2)
        return _mm256_fmadd_pd(a, b, c);
#else
        return _mm256_add_pd(_mm256_mul_pd(a, b), c);
#endif
    }
    RMATH_FORCEINLINE double haddd(__m256d vec) {
        __m128d vlow = _mm256_castpd256_pd128(vec);
        __m128d vhigh = _mm256_extractf128_pd(vec, 1);
        vlow = _mm_add_pd(vlow, vhigh);
        __m128d shuf = _mm_shuffle_pd(vlow, vlow, _MM_SHUFFLE2(0, 1));
        vlow = _mm_add_pd(vlow, shuf);
        return _mm_cvtsd_f64(vlow);
    }
    RMATH_FORCEINLINE auto zerod() { return _mm256_setzero_pd(); }

    // int AVX
    RMATH_FORCEINLINE auto loadui(const int *ptr) { return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr)); }
    RMATH_FORCEINLINE void storeui(int *ptr, __m256i vec) { _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), vec); }
    RMATH_FORCEINLINE auto set1i(int val) { return _mm256_set1_epi32(val); }
    RMATH_FORCEINLINE auto addi(__m256i a, __m256i b) { return _mm256_add_epi32(a, b); }
    RMATH_FORCEINLINE auto subi(__m256i a, __m256i b) { return _mm256_sub_epi32(a, b); }
    RMATH_FORCEINLINE auto muli(__m256i a, __m256i b) {
#if defined(RMATH_AVX2)
        return _mm256_mullo_epi32(a, b);
#else
        __m128i a_lo = _mm_castps_si128(_mm256_castps256_ps128(_mm256_castsi256_ps(a)));
        __m128i a_hi = _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(a), 1));
        __m128i b_lo = _mm_castps_si128(_mm256_castps256_ps128(_mm256_castsi256_ps(b)));
        __m128i b_hi = _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(b), 1));
        a_lo = _mm_mullo_epi32(a_lo, b_lo);
        a_hi = _mm_mullo_epi32(a_hi, b_hi);
        __m256 result = _mm256_castps128_ps256(_mm_castsi128_ps(a_lo));
        result = _mm256_insertf128_ps(result, _mm_castsi128_ps(a_hi), 1);
        return _mm256_castps_si256(result);
#endif
    }
    RMATH_FORCEINLINE int haddi(__m256i vec) {
        __m128i vlow = _mm256_castsi256_si128(vec);
        __m128i vhigh = _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(vec), 1));
        vlow = _mm_add_epi32(vlow, vhigh);
        __m128i hi32  = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(2, 3, 0, 1));
        vlow = _mm_add_epi32(vlow, hi32);
        __m128i hi16  = _mm_shuffle_epi32(vlow, _MM_SHUFFLE(1, 0, 3, 2));
        vlow = _mm_add_epi32(vlow, hi16);
        return _mm_cvtsi128_si32(vlow);
    }
    RMATH_FORCEINLINE auto zeroi() { return _mm256_setzero_si256(); }

#elif defined(RMATH_SSE2)
    // float SSE
    RMATH_FORCEINLINE auto loaduf(const float *ptr) { return _mm_loadu_ps(ptr); }
    RMATH_FORCEINLINE void storeuf(float *ptr, __m128 vec) { _mm_storeu_ps(ptr, vec); }
    RMATH_FORCEINLINE auto set1f(float val) { return _mm_set1_ps(val); }
    RMATH_FORCEINLINE auto addf(__m128 a, __m128 b) { return _mm_add_ps(a, b); }
    RMATH_FORCEINLINE auto subf(__m128 a, __m128 b) { return _mm_sub_ps(a, b); }
    RMATH_FORCEINLINE auto mulf(__m128 a, __m128 b) { return _mm_mul_ps(a, b); }
    RMATH_FORCEINLINE auto divf(__m128 a, __m128 b) { return _mm_div_ps(a, b); }
    RMATH_FORCEINLINE auto fmaddf(__m128 a, __m128 b, __m128 c) {
        return _mm_add_ps(_mm_mul_ps(a, b), c);
    }
    RMATH_FORCEINLINE float haddf(__m128 vec) {
        __m128 shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 3, 0, 1));
        vec = _mm_add_ps(vec, shuf);
        shuf = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 0, 3, 2));
        vec = _mm_add_ps(vec, shuf);
        return _mm_cvtss_f32(vec);
    }
    RMATH_FORCEINLINE auto zerof() { return _mm_setzero_ps(); }

    // double SSE2
    RMATH_FORCEINLINE auto loadud(const double *ptr) { return _mm_loadu_pd(ptr); }
    RMATH_FORCEINLINE void storeud(double *ptr, __m128d vec) { _mm_storeu_pd(ptr, vec); }
    RMATH_FORCEINLINE auto set1d(double val) { return _mm_set1_pd(val); }
    RMATH_FORCEINLINE auto addd(__m128d a, __m128d b) { return _mm_add_pd(a, b); }
    RMATH_FORCEINLINE auto subd(__m128d a, __m128d b) { return _mm_sub_pd(a, b); }
    RMATH_FORCEINLINE auto muld(__m128d a, __m128d b) { return _mm_mul_pd(a, b); }
    RMATH_FORCEINLINE auto divd(__m128d a, __m128d b) { return _mm_div_pd(a, b); }
    RMATH_FORCEINLINE auto fmaddd(__m128d a, __m128d b, __m128d c) {
        return _mm_add_pd(_mm_mul_pd(a, b), c);
    }
    RMATH_FORCEINLINE double haddd(__m128d vec) {
        __m128d shuf = _mm_shuffle_pd(vec, vec, _MM_SHUFFLE2(0, 1));
        vec = _mm_add_pd(vec, shuf);
        return _mm_cvtsd_f64(vec);
    }
    RMATH_FORCEINLINE auto zerod() { return _mm_setzero_pd(); }

    // int SSE2
    RMATH_FORCEINLINE auto loadui(const int *ptr) { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr)); }
    RMATH_FORCEINLINE void storeui(int *ptr, __m128i vec) { _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), vec); }
    RMATH_FORCEINLINE auto set1i(int val) { return _mm_set1_epi32(val); }
    RMATH_FORCEINLINE auto addi(__m128i a, __m128i b) { return _mm_add_epi32(a, b); }
    RMATH_FORCEINLINE auto subi(__m128i a, __m128i b) { return _mm_sub_epi32(a, b); }
    RMATH_FORCEINLINE auto muli(__m128i a, __m128i b) {
#if defined(RMATH_SSE42)
        return _mm_mullo_epi32(a, b);
#else
        __m128i tmp1 = _mm_mul_epu32(a, b);
        __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
        return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)),
                                   _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0)));
#endif
    }
    RMATH_FORCEINLINE int haddi(__m128i vec) {
        __m128i hi64  = _mm_unpackhi_epi64(vec, vec);
        vec = _mm_add_epi32(vec, hi64);
        __m128i hi32  = _mm_shuffle_epi32(vec, _MM_SHUFFLE(2, 3, 0, 1));
        vec = _mm_add_epi32(vec, hi32);
        return _mm_cvtsi128_si32(vec);
    }
    RMATH_FORCEINLINE auto zeroi() { return _mm_setzero_si128(); }
#endif

    // Generic dispatch wrappers
    template <typename T>
    RMATH_FORCEINLINE auto loadu(const T *ptr) {
        if constexpr (std::is_same_v<T, float>) return loaduf(ptr);
        else if constexpr (std::is_same_v<T, double>) return loadud(ptr);
        else if constexpr (std::is_same_v<T, int>) return loadui(ptr);
    }

    template <typename T>
    RMATH_FORCEINLINE void storeu(T *ptr, auto vec) {
        if constexpr (std::is_same_v<T, float>) storeuf(ptr, vec);
        else if constexpr (std::is_same_v<T, double>) storeud(ptr, vec);
        else if constexpr (std::is_same_v<T, int>) storeui(ptr, vec);
    }

    template <typename T>
    RMATH_FORCEINLINE auto set1(T val) {
        if constexpr (std::is_same_v<T, float>) return set1f(val);
        else if constexpr (std::is_same_v<T, double>) return set1d(val);
        else if constexpr (std::is_same_v<T, int>) return set1i(val);
    }

    template <typename T>
    RMATH_FORCEINLINE auto add(auto a, auto b) {
        if constexpr (std::is_same_v<T, float>) return addf(a, b);
        else if constexpr (std::is_same_v<T, double>) return addd(a, b);
        else if constexpr (std::is_same_v<T, int>) return addi(a, b);
    }

    template <typename T>
    RMATH_FORCEINLINE auto sub(auto a, auto b) {
        if constexpr (std::is_same_v<T, float>) return subf(a, b);
        else if constexpr (std::is_same_v<T, double>) return subd(a, b);
        else if constexpr (std::is_same_v<T, int>) return subi(a, b);
    }

    template <typename T>
    RMATH_FORCEINLINE auto mul(auto a, auto b) {
        if constexpr (std::is_same_v<T, float>) return mulf(a, b);
        else if constexpr (std::is_same_v<T, double>) return muld(a, b);
        else if constexpr (std::is_same_v<T, int>) return muli(a, b);
    }

    template <typename T>
    RMATH_FORCEINLINE auto div(auto a, auto b) {
        if constexpr (std::is_same_v<T, float>) return divf(a, b);
        else if constexpr (std::is_same_v<T, double>) return divd(a, b);
    }

    template <typename T>
    RMATH_FORCEINLINE auto fmadd(auto a, auto b, auto c) {
        if constexpr (std::is_same_v<T, float>) return fmaddf(a, b, c);
        else if constexpr (std::is_same_v<T, double>) return fmaddd(a, b, c);
        else if constexpr (std::is_same_v<T, int>) return addi(muli(a, b), c);
    }

    template <typename T>
    RMATH_FORCEINLINE T hadd(auto vec) {
        if constexpr (std::is_same_v<T, float>) return haddf(vec);
        else if constexpr (std::is_same_v<T, double>) return haddd(vec);
        else if constexpr (std::is_same_v<T, int>) return haddi(vec);
    }

    template <typename T>
    RMATH_FORCEINLINE auto zero() {
        if constexpr (std::is_same_v<T, float>) return zerof();
        else if constexpr (std::is_same_v<T, double>) return zerod();
        else if constexpr (std::is_same_v<T, int>) return zeroi();
    }

} // namespace Detail::simd

// 便捷别名：simd 实现已下沉至 Detail::simd，库内部统一使用该别名引用
namespace simd = Detail::simd;

}