#pragma once

#include "SIMDBase.hpp"

#if defined(RSIMD_SSE2) || defined(RSIMD_AVX)
    #include "SIMDx86.hpp"
#elif defined(RSIMD_NEON)
    #include "SIMDARM.hpp"
#else
    #error "Unsupported SIMD platform. Only x86 (SSE2/AVX) and ARM (NEON) are supported."
#endif

namespace RandomEngine::Platform::SIMD
{

// =========================================================================
// 平台无关类型别名
// =========================================================================

#if defined(RSIMD_AVX)
    using FloatVec = __m256;
    using DoubleVec = __m256d;
    using IntVec = __m256i;
#elif defined(RSIMD_SSE2)
    using FloatVec = __m128;
    using DoubleVec = __m128d;
    using IntVec = __m128i;
#elif defined(RSIMD_NEON)
    using FloatVec = float32x4_t;
    using DoubleVec = float64x2_t;
    using IntVec = int32x4_t;
#endif

// =========================================================================
// 平台无关 SIMD 向量包裹类型
// =========================================================================

template <typename T>
struct alignas(SIMDAlignment) SIMDVec
{
    using VecType = std::conditional_t<
        std::is_same_v<T, float>, FloatVec,
        std::conditional_t<
            std::is_same_v<T, double>, DoubleVec,
            IntVec
        >
    >;

    VecType v;

    static constexpr size_t Width = SIMDWidth<T>;

    RSIMD_FORCEINLINE SIMDVec() : v(Zero<T>()) {}
    RSIMD_FORCEINLINE SIMDVec(VecType vec) : v(vec) {}
    RSIMD_FORCEINLINE SIMDVec(T scalar) : v(Set1<T>(scalar)) {}

    RSIMD_FORCEINLINE static SIMDVec Load(const T* ptr) {
        return SIMDVec(::RandomEngine::Platform::SIMD::Load<T>(ptr));
    }
    RSIMD_FORCEINLINE static SIMDVec LoadU(const T* ptr) {
        return SIMDVec(::RandomEngine::Platform::SIMD::LoadU<T>(ptr));
    }
    RSIMD_FORCEINLINE void Store(T* ptr) const {
        ::RandomEngine::Platform::SIMD::Store<T>(ptr, v);
    }
    RSIMD_FORCEINLINE void StoreU(T* ptr) const {
        ::RandomEngine::Platform::SIMD::StoreU<T>(ptr, v);
    }

    // 算术
    RSIMD_FORCEINLINE SIMDVec operator+(SIMDVec other) const { return SIMDVec(Add<T>(v, other.v)); }
    RSIMD_FORCEINLINE SIMDVec operator-(SIMDVec other) const { return SIMDVec(Sub<T>(v, other.v)); }
    RSIMD_FORCEINLINE SIMDVec operator*(SIMDVec other) const { return SIMDVec(Mul<T>(v, other.v)); }
    RSIMD_FORCEINLINE SIMDVec operator/(SIMDVec other) const { return SIMDVec(Div<T>(v, other.v)); }
    RSIMD_FORCEINLINE SIMDVec operator-() const { return SIMDVec(Neg<T>(v)); }

    RSIMD_FORCEINLINE SIMDVec& operator+=(SIMDVec other) { v = Add<T>(v, other.v); return *this; }
    RSIMD_FORCEINLINE SIMDVec& operator-=(SIMDVec other) { v = Sub<T>(v, other.v); return *this; }
    RSIMD_FORCEINLINE SIMDVec& operator*=(SIMDVec other) { v = Mul<T>(v, other.v); return *this; }
    RSIMD_FORCEINLINE SIMDVec& operator/=(SIMDVec other) { v = Div<T>(v, other.v); return *this; }

    // 比较 (返回 SIMDVec<int32_t>)
    RSIMD_FORCEINLINE auto CmpEq(SIMDVec other) const { return CmpEQ<T>(v, other.v); }
    RSIMD_FORCEINLINE auto CmpNe(SIMDVec other) const { return CmpNE<T>(v, other.v); }
    RSIMD_FORCEINLINE auto CmpLt(SIMDVec other) const { return CmpLT<T>(v, other.v); }
    RSIMD_FORCEINLINE auto CmpLe(SIMDVec other) const { return CmpLE<T>(v, other.v); }
    RSIMD_FORCEINLINE auto CmpGt(SIMDVec other) const { return CmpGT<T>(v, other.v); }
    RSIMD_FORCEINLINE auto CmpGe(SIMDVec other) const { return CmpGE<T>(v, other.v); }

    // 数学
    RSIMD_FORCEINLINE SIMDVec Sqrt() const { return SIMDVec(::RandomEngine::Platform::SIMD::Sqrt<T>(v)); }
    RSIMD_FORCEINLINE SIMDVec RSqrt() const { return SIMDVec(::RandomEngine::Platform::SIMD::RSqrt<T>(v)); }
    RSIMD_FORCEINLINE SIMDVec Rcp() const { return SIMDVec(::RandomEngine::Platform::SIMD::Rcp<T>(v)); }
    RSIMD_FORCEINLINE SIMDVec Min(SIMDVec other) const { return SIMDVec(::RandomEngine::Platform::SIMD::Min<T>(v, other.v)); }
    RSIMD_FORCEINLINE SIMDVec Max(SIMDVec other) const { return SIMDVec(::RandomEngine::Platform::SIMD::Max<T>(v, other.v)); }
    RSIMD_FORCEINLINE SIMDVec Abs() const { return SIMDVec(::RandomEngine::Platform::SIMD::Abs<T>(v)); }
    RSIMD_FORCEINLINE SIMDVec Floor() const { return SIMDVec(::RandomEngine::Platform::SIMD::Floor<T>(v)); }
    RSIMD_FORCEINLINE SIMDVec Ceil() const { return SIMDVec(::RandomEngine::Platform::SIMD::Ceil<T>(v)); }
    RSIMD_FORCEINLINE SIMDVec Round() const { return SIMDVec(::RandomEngine::Platform::SIMD::Round<T>(v)); }

    // FMA
    RSIMD_FORCEINLINE static SIMDVec FMAdd(SIMDVec a, SIMDVec b, SIMDVec c) {
        return SIMDVec(::RandomEngine::Platform::SIMD::FMAdd<T>(a.v, b.v, c.v));
    }
    RSIMD_FORCEINLINE static SIMDVec FMSub(SIMDVec a, SIMDVec b, SIMDVec c) {
        return SIMDVec(::RandomEngine::Platform::SIMD::FMSub<T>(a.v, b.v, c.v));
    }

    // 水平归约
    RSIMD_FORCEINLINE T HSum() const { return HAdd<T>(v); }
    RSIMD_FORCEINLINE T HMin() const { return ::RandomEngine::Platform::SIMD::HMin<T>(v); }
    RSIMD_FORCEINLINE T HMax() const { return ::RandomEngine::Platform::SIMD::HMax<T>(v); }
};

// 便捷类型别名
using SIMDVec4f = SIMDVec<float>;
using SIMDVec4d = SIMDVec<double>;
using SIMDVec4i = SIMDVec<int32_t>;

} // namespace RandomEngine::Platform::SIMD