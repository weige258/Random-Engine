#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <new>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

#if defined(_MSC_VER)
    #include <immintrin.h>
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
#endif

namespace RandomEngine::Platform::SIMD
{

    // =========================================================================
    // CPU 特性检测 (编译期)
    // =========================================================================

#if defined(__AVX2__)
    #define RSIMD_AVX2
    #define RSIMD_AVX
    #define RSIMD_FMA
    #define RSIMD_SSE42
    #define RSIMD_SSE3
    #define RSIMD_SSE2
#elif defined(__AVX__)
    #define RSIMD_AVX
    #define RSIMD_SSE42
    #define RSIMD_SSE3
    #define RSIMD_SSE2
#elif defined(__SSE4_2__)
    #define RSIMD_SSE42
    #define RSIMD_SSE3
    #define RSIMD_SSE2
#elif defined(__SSSE3__)
    #define RSIMD_SSE3
    #define RSIMD_SSE2
#elif defined(__SSE2__)
    #define RSIMD_SSE2
#endif

#if defined(__FMA__)
    #define RSIMD_FMA
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define RSIMD_NEON
#endif

    // =========================================================================
    // 强制内联宏 (避免 __m256 等类型 ABI 问题)
    // =========================================================================

#if defined(_MSC_VER)
    #define RSIMD_FORCEINLINE __forceinline
#else
    #define RSIMD_FORCEINLINE inline __attribute__((always_inline))
#endif

    // =========================================================================
    // SIMD 宽度 (每个寄存器可容纳的元素数)
    // =========================================================================

    template <typename T>
    inline constexpr size_t SIMDWidth = 1;

#if defined(RSIMD_AVX)
    template <> inline constexpr size_t SIMDWidth<float> = 8;
    template <> inline constexpr size_t SIMDWidth<double> = 4;
    template <> inline constexpr size_t SIMDWidth<int32_t> = 8;
    template <> inline constexpr size_t SIMDWidth<uint32_t> = 8;
    template <> inline constexpr size_t SIMDWidth<int64_t> = 4;
    template <> inline constexpr size_t SIMDWidth<uint64_t> = 4;
#elif defined(RSIMD_SSE2)
    template <> inline constexpr size_t SIMDWidth<float> = 4;
    template <> inline constexpr size_t SIMDWidth<double> = 2;
    template <> inline constexpr size_t SIMDWidth<int32_t> = 4;
    template <> inline constexpr size_t SIMDWidth<uint32_t> = 4;
    template <> inline constexpr size_t SIMDWidth<int64_t> = 2;
    template <> inline constexpr size_t SIMDWidth<uint64_t> = 2;
#elif defined(RSIMD_NEON)
    template <> inline constexpr size_t SIMDWidth<float> = 4;
    template <> inline constexpr size_t SIMDWidth<double> = 2;
    template <> inline constexpr size_t SIMDWidth<int32_t> = 4;
    template <> inline constexpr size_t SIMDWidth<uint32_t> = 4;
    template <> inline constexpr size_t SIMDWidth<int64_t> = 2;
    template <> inline constexpr size_t SIMDWidth<uint64_t> = 2;
#endif

    // =========================================================================
    // 对齐要求
    // =========================================================================

#if defined(RSIMD_AVX)
    inline constexpr size_t SIMDAlignment = 32;
#elif defined(RSIMD_SSE2) || defined(RSIMD_NEON)
    inline constexpr size_t SIMDAlignment = 16;
#else
    inline constexpr size_t SIMDAlignment = alignof(std::max_align_t);
#endif

    // =========================================================================
    // 对齐内存分配/释放
    // =========================================================================

    RSIMD_FORCEINLINE void* AlignedAlloc(size_t size, size_t alignment = SIMDAlignment)
    {
    #if defined(_MSC_VER)
        return _aligned_malloc(size, alignment);
    #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        return aligned_alloc(alignment, (size + alignment - 1) & ~(alignment - 1));
    #else
        // portable fallback: allocate extra memory and align manually
        void* orig = std::malloc(size + alignment - 1 + sizeof(void*));
        if (!orig) return nullptr;
        uintptr_t ptr = reinterpret_cast<uintptr_t>(orig) + sizeof(void*);
        uintptr_t aligned = (ptr + alignment - 1) & ~(alignment - 1);
        void** store = reinterpret_cast<void**>(aligned);
        store[-1] = orig;
        return reinterpret_cast<void*>(aligned);
    #endif
    }

    RSIMD_FORCEINLINE void AlignedFree(void* ptr)
    {
    #if defined(_MSC_VER)
        _aligned_free(ptr);
    #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        std::free(ptr);
    #else
        if (!ptr) return;
        void* orig = reinterpret_cast<void**>(ptr)[-1];
        std::free(orig);
    #endif
    }

    template <typename T>
    RSIMD_FORCEINLINE T* AlignedNew(size_t count = 1)
    {
        void* mem = AlignedAlloc(sizeof(T) * count, alignof(T) > SIMDAlignment ? alignof(T) : SIMDAlignment);
        if (!mem) return nullptr;
        for (size_t i = 0; i < count; ++i)
            new (static_cast<T*>(mem) + i) T();
        return static_cast<T*>(mem);
    }

    template <typename T>
    RSIMD_FORCEINLINE void AlignedDelete(T* ptr, size_t count = 1)
    {
        if (!ptr) return;
        for (size_t i = 0; i < count; ++i)
            ptr[i].~T();
        AlignedFree(ptr);
    }

    // =========================================================================
    // 类型支持检测
    // =========================================================================

    template <typename T>
    inline constexpr bool SupportsSIMD = false;

    template <> inline constexpr bool SupportsSIMD<float> = true;
    template <> inline constexpr bool SupportsSIMD<double> = true;
    template <> inline constexpr bool SupportsSIMD<int32_t> = true;
    template <> inline constexpr bool SupportsSIMD<uint32_t> = true;
    template <> inline constexpr bool SupportsSIMD<int64_t> = true;
    template <> inline constexpr bool SupportsSIMD<uint64_t> = true;

    // =========================================================================
    // Mask 类型 (比较结果)
    // =========================================================================

#if defined(RSIMD_AVX)
    using MaskFloat  = __m256;
    using MaskDouble = __m256d;
    using MaskInt32  = __m256i;
    using MaskInt64  = __m256i;
#elif defined(RSIMD_SSE2)
    using MaskFloat  = __m128;
    using MaskDouble = __m128d;
    using MaskInt32  = __m128i;
    using MaskInt64  = __m128i;
#endif

    // =========================================================================
    // 前向声明：泛型分发函数 (实现在 SIMDx86.hpp / SIMDARM.hpp 中)
    // 注意：这些函数的实际定义在平台实现文件中，通过 SIMD.hpp 统一引入
    // =========================================================================

    // --- 内存操作 ---
    template <typename T> RSIMD_FORCEINLINE auto Load(const T* ptr);
    template <typename T> RSIMD_FORCEINLINE auto LoadU(const T* ptr);
    template <typename T> RSIMD_FORCEINLINE void Store(T* ptr, auto vec);
    template <typename T> RSIMD_FORCEINLINE void StoreU(T* ptr, auto vec);
    template <typename T> RSIMD_FORCEINLINE auto Set1(T val);
    template <typename T> RSIMD_FORCEINLINE auto Set(T v0, T v1, T v2, T v3);
    template <typename T> RSIMD_FORCEINLINE auto Set(T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7);
    template <typename T> RSIMD_FORCEINLINE auto Zero();

    // --- 算术运算 ---
    template <typename T> RSIMD_FORCEINLINE auto Add(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Sub(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Mul(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Div(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Neg(auto a);
    template <typename T> RSIMD_FORCEINLINE auto Abs(auto a);

    // --- FMA (Fused Multiply-Add) ---
    template <typename T> RSIMD_FORCEINLINE auto FMAdd(auto a, auto b, auto c);
    template <typename T> RSIMD_FORCEINLINE auto FMSub(auto a, auto b, auto c);
    template <typename T> RSIMD_FORCEINLINE auto FNMAdd(auto a, auto b, auto c);
    template <typename T> RSIMD_FORCEINLINE auto FNMSub(auto a, auto b, auto c);

    // --- 比较运算 ---
    template <typename T> RSIMD_FORCEINLINE auto CmpEQ(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto CmpNE(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto CmpLT(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto CmpLE(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto CmpGT(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto CmpGE(auto a, auto b);

    // --- 逻辑运算 ---
    template <typename T> RSIMD_FORCEINLINE auto And(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Or(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Xor(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto AndNot(auto a, auto b);

    // --- 混合/选择 ---
    template <typename T> RSIMD_FORCEINLINE auto Blend(auto a, auto b, auto mask);
    template <typename T> RSIMD_FORCEINLINE auto Select(auto mask, auto a, auto b);

    // --- 数学函数 ---
    template <typename T> RSIMD_FORCEINLINE auto Sqrt(auto a);
    template <typename T> RSIMD_FORCEINLINE auto RSqrt(auto a);
    template <typename T> RSIMD_FORCEINLINE auto Rcp(auto a);
    template <typename T> RSIMD_FORCEINLINE auto Min(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Max(auto a, auto b);
    template <typename T> RSIMD_FORCEINLINE auto Floor(auto a);
    template <typename T> RSIMD_FORCEINLINE auto Ceil(auto a);
    template <typename T> RSIMD_FORCEINLINE auto Round(auto a);

    // --- 水平归约 ---
    template <typename T> RSIMD_FORCEINLINE T HAdd(auto vec);
    template <typename T> RSIMD_FORCEINLINE T HMin(auto vec);
    template <typename T> RSIMD_FORCEINLINE T HMax(auto vec);

    // --- 类型转换 ---
    template <typename T> RSIMD_FORCEINLINE auto ConvertToFloat(auto a);
    template <typename T> RSIMD_FORCEINLINE auto ConvertToInt(auto a);

    // =========================================================================
    // 便捷类型别名
    // =========================================================================

#if defined(RSIMD_AVX)
    using VecF32 = __m256;
    using VecF64 = __m256d;
    using VecI32 = __m256i;
    using VecI64 = __m256i;
    using VecU32 = __m256i;
    using VecU64 = __m256i;
#elif defined(RSIMD_SSE2)
    using VecF32 = __m128;
    using VecF64 = __m128d;
    using VecI32 = __m128i;
    using VecI64 = __m128i;
    using VecU32 = __m128i;
    using VecU64 = __m128i;
#elif defined(RSIMD_NEON)
    using VecF32 = float32x4_t;
    using VecF64 = float64x2_t;
    using VecI32 = int32x4_t;
    using VecI64 = int64x2_t;
    using VecU32 = uint32x4_t;
    using VecU64 = uint64x2_t;
#endif

} // namespace RandomEngine::Platform::SIMD