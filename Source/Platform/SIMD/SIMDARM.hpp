#pragma once

#include "SIMDBase.hpp"

#if defined(RSIMD_NEON)
    #include <arm_neon.h>
#endif

#include <cmath>

namespace RandomEngine::Platform::SIMD
{

#if defined(RSIMD_NEON)

// =========================================================================
// float 实现 (NEON)
// =========================================================================

RSIMD_FORCEINLINE auto Loadf(const float* ptr) { return vld1q_f32(ptr); }
RSIMD_FORCEINLINE auto LoadUf(const float* ptr) { return vld1q_f32(ptr); }
RSIMD_FORCEINLINE void Storef(float* ptr, float32x4_t vec) { vst1q_f32(ptr, vec); }
RSIMD_FORCEINLINE void StoreUf(float* ptr, float32x4_t vec) { vst1q_f32(ptr, vec); }
RSIMD_FORCEINLINE auto Set1f(float val) { return vdupq_n_f32(val); }
RSIMD_FORCEINLINE auto Zerof() { return vdupq_n_f32(0.0f); }
RSIMD_FORCEINLINE auto Setf(float v0, float v1, float v2, float v3) {
    float32x4_t result;
    result = vsetq_lane_f32(v0, result, 0);
    result = vsetq_lane_f32(v1, result, 1);
    result = vsetq_lane_f32(v2, result, 2);
    result = vsetq_lane_f32(v3, result, 3);
    return result;
}

RSIMD_FORCEINLINE auto Addf(float32x4_t a, float32x4_t b) { return vaddq_f32(a, b); }
RSIMD_FORCEINLINE auto Subf(float32x4_t a, float32x4_t b) { return vsubq_f32(a, b); }
RSIMD_FORCEINLINE auto Mulf(float32x4_t a, float32x4_t b) { return vmulq_f32(a, b); }
RSIMD_FORCEINLINE auto Divf(float32x4_t a, float32x4_t b) {
    float32x4_t rec = vrecpeq_f32(b);
    rec = vmulq_f32(vrecpsq_f32(b, rec), rec);
    return vmulq_f32(a, rec);
}
RSIMD_FORCEINLINE auto Negf(float32x4_t a) { return vnegq_f32(a); }
RSIMD_FORCEINLINE auto Absf(float32x4_t a) { return vabsq_f32(a); }

RSIMD_FORCEINLINE auto FMAddf(float32x4_t a, float32x4_t b, float32x4_t c) {
#if defined(__ARM_FEATURE_FMA)
    return vfmaq_f32(c, a, b);
#else
    return vaddq_f32(vmulq_f32(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FMSubf(float32x4_t a, float32x4_t b, float32x4_t c) {
#if defined(__ARM_FEATURE_FMA)
    return vfmsq_f32(c, a, b);
#else
    return vsubq_f32(vmulq_f32(a, b), c);
#endif
}
RSIMD_FORCEINLINE auto FNMAddf(float32x4_t a, float32x4_t b, float32x4_t c) {
    return vsubq_f32(c, vmulq_f32(a, b));
}
RSIMD_FORCEINLINE auto FNMSubf(float32x4_t a, float32x4_t b, float32x4_t c) {
    return vnegq_f32(vaddq_f32(vmulq_f32(a, b), c));
}

RSIMD_FORCEINLINE auto CmpEQf(float32x4_t a, float32x4_t b) { return vceqq_f32(a, b); }
RSIMD_FORCEINLINE auto CmpNEf(float32x4_t a, float32x4_t b) {
    return vmvnq_u32(vceqq_f32(a, b));
}
RSIMD_FORCEINLINE auto CmpLTf(float32x4_t a, float32x4_t b) { return vcltq_f32(a, b); }
RSIMD_FORCEINLINE auto CmpLEf(float32x4_t a, float32x4_t b) { return vcleq_f32(a, b); }
RSIMD_FORCEINLINE auto CmpGTf(float32x4_t a, float32x4_t b) { return vcgtq_f32(a, b); }
RSIMD_FORCEINLINE auto CmpGEf(float32x4_t a, float32x4_t b) { return vcgeq_f32(a, b); }

RSIMD_FORCEINLINE auto Andf(float32x4_t a, float32x4_t b) {
    return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}
RSIMD_FORCEINLINE auto Orf(float32x4_t a, float32x4_t b) {
    return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}
RSIMD_FORCEINLINE auto Xorf(float32x4_t a, float32x4_t b) {
    return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}
RSIMD_FORCEINLINE auto AndNotf(float32x4_t a, float32x4_t b) {
    return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(b), vreinterpretq_u32_f32(a)));
}

RSIMD_FORCEINLINE auto Blendf(float32x4_t a, float32x4_t b, uint32x4_t mask) {
    return vbslq_f32(mask, b, a);
}
RSIMD_FORCEINLINE auto Selectf(uint32x4_t mask, float32x4_t a, float32x4_t b) {
    return vbslq_f32(mask, a, b);
}

RSIMD_FORCEINLINE auto Sqrtf(float32x4_t a) { return vsqrtq_f32(a); }
RSIMD_FORCEINLINE auto RSqrtf(float32x4_t a) {
    float32x4_t r = vrsqrteq_f32(a);
    r = vmulq_f32(vrsqrtsq_f32(vmulq_f32(a, r), r), r);
    return r;
}
RSIMD_FORCEINLINE auto Rcpf(float32x4_t a) {
    float32x4_t r = vrecpeq_f32(a);
    r = vmulq_f32(vrecpsq_f32(a, r), r);
    return r;
}
RSIMD_FORCEINLINE auto Minf(float32x4_t a, float32x4_t b) { return vminq_f32(a, b); }
RSIMD_FORCEINLINE auto Maxf(float32x4_t a, float32x4_t b) { return vmaxq_f32(a, b); }
RSIMD_FORCEINLINE auto Floorf(float32x4_t a) { return vrndmq_f32(a); }
RSIMD_FORCEINLINE auto Ceilf(float32x4_t a) { return vrndpq_f32(a); }
RSIMD_FORCEINLINE auto Roundf(float32x4_t a) { return vrndnq_f32(a); }

RSIMD_FORCEINLINE float HAddf(float32x4_t vec) {
    float32x2_t vlow = vget_low_f32(vec);
    float32x2_t vhigh = vget_high_f32(vec);
    vlow = vadd_f32(vlow, vhigh);
    vlow = vpadd_f32(vlow, vlow);
    return vget_lane_f32(vlow, 0);
}
RSIMD_FORCEINLINE float HMinf(float32x4_t vec) {
    float32x2_t vlow = vget_low_f32(vec);
    float32x2_t vhigh = vget_high_f32(vec);
    vlow = vmin_f32(vlow, vhigh);
    vlow = vpmin_f32(vlow, vlow);
    return vget_lane_f32(vlow, 0);
}
RSIMD_FORCEINLINE float HMaxf(float32x4_t vec) {
    float32x2_t vlow = vget_low_f32(vec);
    float32x2_t vhigh = vget_high_f32(vec);
    vlow = vmax_f32(vlow, vhigh);
    vlow = vpmax_f32(vlow, vlow);
    return vget_lane_f32(vlow, 0);
}

RSIMD_FORCEINLINE auto Shufflef(float32x4_t a, float32x4_t b, int imm8) {
    float32x4_t result;
    result = vsetq_lane_f32(vgetq_lane_f32((imm8 & 1) ? b : a, (imm8 >> 0) & 1), result, 0);
    result = vsetq_lane_f32(vgetq_lane_f32((imm8 & 2) ? b : a, (imm8 >> 1) & 1), result, 1);
    result = vsetq_lane_f32(vgetq_lane_f32((imm8 & 4) ? b : a, (imm8 >> 2) & 1), result, 2);
    result = vsetq_lane_f32(vgetq_lane_f32((imm8 & 8) ? b : a, (imm8 >> 3) & 1), result, 3);
    return result;
}
RSIMD_FORCEINLINE auto Permutef(float32x4_t a, float32x4_t, int) { return a; }
RSIMD_FORCEINLINE auto BroadcastLanef(float32x4_t a, int lane) {
    float32x2_t half = (lane < 2) ? vget_low_f32(a) : vget_high_f32(a);
    return vdupq_lane_f32(half, lane & 1);
}

RSIMD_FORCEINLINE auto ConvertToFloatf(int32x4_t a) { return vcvtq_f32_s32(a); }
RSIMD_FORCEINLINE auto ConvertToIntf(float32x4_t a) { return vcvtq_s32_f32(a); }

// =========================================================================
// double 实现 (NEON - AArch64, 2-wide)
// =========================================================================

RSIMD_FORCEINLINE auto Loadd(const double* ptr) { return vld1q_f64(ptr); }
RSIMD_FORCEINLINE auto LoadUd(const double* ptr) { return vld1q_f64(ptr); }
RSIMD_FORCEINLINE void Stored(double* ptr, float64x2_t vec) { vst1q_f64(ptr, vec); }
RSIMD_FORCEINLINE void StoreUd(double* ptr, float64x2_t vec) { vst1q_f64(ptr, vec); }
RSIMD_FORCEINLINE auto Set1d(double val) { return vdupq_n_f64(val); }
RSIMD_FORCEINLINE auto Zerod() { return vdupq_n_f64(0.0); }
RSIMD_FORCEINLINE auto Setd(double v0, double v1) {
    float64x2_t result;
    result = vsetq_lane_f64(v0, result, 0);
    result = vsetq_lane_f64(v1, result, 1);
    return result;
}

RSIMD_FORCEINLINE auto Addd(float64x2_t a, float64x2_t b) { return vaddq_f64(a, b); }
RSIMD_FORCEINLINE auto Subd(float64x2_t a, float64x2_t b) { return vsubq_f64(a, b); }
RSIMD_FORCEINLINE auto Muld(float64x2_t a, float64x2_t b) { return vmulq_f64(a, b); }
RSIMD_FORCEINLINE auto Divd(float64x2_t a, float64x2_t b) {
    float64x2_t rec = vrecpeq_f64(b);
    return vmulq_f64(a, rec);
}
RSIMD_FORCEINLINE auto Negd(float64x2_t a) { return vnegq_f64(a); }
RSIMD_FORCEINLINE auto Absd(float64x2_t a) { return vabsq_f64(a); }

RSIMD_FORCEINLINE auto CmpEQd(float64x2_t a, float64x2_t b) { return vceqq_f64(a, b); }
RSIMD_FORCEINLINE auto CmpNEd(float64x2_t a, float64x2_t b) {
    uint64x2_t eq = vceqq_f64(a, b);
    return veorq_u64(eq, vdupq_n_u64(~0ULL));
}
RSIMD_FORCEINLINE auto CmpLTd(float64x2_t a, float64x2_t b) { return vcltq_f64(a, b); }
RSIMD_FORCEINLINE auto CmpLEd(float64x2_t a, float64x2_t b) { return vcleq_f64(a, b); }
RSIMD_FORCEINLINE auto CmpGTd(float64x2_t a, float64x2_t b) { return vcgtq_f64(a, b); }
RSIMD_FORCEINLINE auto CmpGEd(float64x2_t a, float64x2_t b) { return vcgeq_f64(a, b); }

RSIMD_FORCEINLINE auto Andd(float64x2_t a, float64x2_t b) {
    return vreinterpretq_f64_u64(vandq_u64(vreinterpretq_u64_f64(a), vreinterpretq_u64_f64(b)));
}
RSIMD_FORCEINLINE auto Ord(float64x2_t a, float64x2_t b) {
    return vreinterpretq_f64_u64(vorrq_u64(vreinterpretq_u64_f64(a), vreinterpretq_u64_f64(b)));
}
RSIMD_FORCEINLINE auto Xord(float64x2_t a, float64x2_t b) {
    return vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(a), vreinterpretq_u64_f64(b)));
}
RSIMD_FORCEINLINE auto AndNotd(float64x2_t a, float64x2_t b) {
    return vreinterpretq_f64_u64(vbicq_u64(vreinterpretq_u64_f64(b), vreinterpretq_u64_f64(a)));
}

RSIMD_FORCEINLINE auto Blendd(float64x2_t a, float64x2_t b, uint64x2_t mask) {
    return vbslq_f64(mask, b, a);
}
RSIMD_FORCEINLINE auto Selectd(uint64x2_t mask, float64x2_t a, float64x2_t b) {
    return vbslq_f64(mask, a, b);
}

RSIMD_FORCEINLINE auto Sqrtd(float64x2_t a) { return vsqrtq_f64(a); }
RSIMD_FORCEINLINE auto Mind(float64x2_t a, float64x2_t b) { return vminq_f64(a, b); }
RSIMD_FORCEINLINE auto Maxd(float64x2_t a, float64x2_t b) { return vmaxq_f64(a, b); }
RSIMD_FORCEINLINE auto Floord(float64x2_t a) { return vrndmq_f64(a); }
RSIMD_FORCEINLINE auto Ceild(float64x2_t a) { return vrndpq_f64(a); }
RSIMD_FORCEINLINE auto Roundd(float64x2_t a) { return vrndnq_f64(a); }

RSIMD_FORCEINLINE double HAddd(float64x2_t vec) {
    return vgetq_lane_f64(vec, 0) + vgetq_lane_f64(vec, 1);
}
RSIMD_FORCEINLINE double HMind(float64x2_t vec) {
    return vgetq_lane_f64(vminq_f64(vec, vextq_f64(vec, vec, 1)), 0);
}
RSIMD_FORCEINLINE double HMaxd(float64x2_t vec) {
    return vgetq_lane_f64(vmaxq_f64(vec, vextq_f64(vec, vec, 1)), 0);
}

RSIMD_FORCEINLINE auto Shuffled(float64x2_t a, float64x2_t b, int imm8) {
    float64x2_t result;
    result = vsetq_lane_f64(vgetq_lane_f64((imm8 & 1) ? b : a, (imm8 >> 0) & 1), result, 0);
    result = vsetq_lane_f64(vgetq_lane_f64((imm8 & 2) ? b : a, (imm8 >> 1) & 1), result, 1);
    return result;
}
RSIMD_FORCEINLINE auto Permuted(float64x2_t a, float64x2_t, int) { return a; }
RSIMD_FORCEINLINE auto BroadcastLanef(float64x2_t a, int lane) {
    return vdupq_lane_f64(vget_low_f64(a), lane);
}

RSIMD_FORCEINLINE auto ConvertToFloatd(int32x4_t a) { return vcvtq_f64_s32(a); }
RSIMD_FORCEINLINE auto ConvertToIntd(float64x2_t a) { return vcvtq_s32_f64(a); }

// =========================================================================
// int32_t 实现 (NEON)
// =========================================================================

RSIMD_FORCEINLINE auto Loadi(const int32_t* ptr) { return vld1q_s32(ptr); }
RSIMD_FORCEINLINE auto LoadUi(const int32_t* ptr) { return vld1q_s32(ptr); }
RSIMD_FORCEINLINE void Storei(int32_t* ptr, int32x4_t vec) { vst1q_s32(ptr, vec); }
RSIMD_FORCEINLINE void StoreUi(int32_t* ptr, int32x4_t vec) { vst1q_s32(ptr, vec); }
RSIMD_FORCEINLINE auto Set1i(int32_t val) { return vdupq_n_s32(val); }
RSIMD_FORCEINLINE auto Zeroi() { return vdupq_n_s32(0); }
RSIMD_FORCEINLINE auto Seti(int32_t v0, int32_t v1, int32_t v2, int32_t v3) {
    int32x4_t result;
    result = vsetq_lane_s32(v0, result, 0);
    result = vsetq_lane_s32(v1, result, 1);
    result = vsetq_lane_s32(v2, result, 2);
    result = vsetq_lane_s32(v3, result, 3);
    return result;
}

RSIMD_FORCEINLINE auto Addi(int32x4_t a, int32x4_t b) { return vaddq_s32(a, b); }
RSIMD_FORCEINLINE auto Subi(int32x4_t a, int32x4_t b) { return vsubq_s32(a, b); }
RSIMD_FORCEINLINE auto Muli(int32x4_t a, int32x4_t b) { return vmulq_s32(a, b); }
RSIMD_FORCEINLINE auto Negi(int32x4_t a) { return vnegq_s32(a); }
RSIMD_FORCEINLINE auto Absi(int32x4_t a) { return vabsq_s32(a); }

RSIMD_FORCEINLINE auto CmpEQi(int32x4_t a, int32x4_t b) { return vceqq_s32(a, b); }
RSIMD_FORCEINLINE auto CmpLTi(int32x4_t a, int32x4_t b) { return vcltq_s32(a, b); }
RSIMD_FORCEINLINE auto CmpGTi(int32x4_t a, int32x4_t b) { return vcgtq_s32(a, b); }

RSIMD_FORCEINLINE auto Mini(int32x4_t a, int32x4_t b) { return vminq_s32(a, b); }
RSIMD_FORCEINLINE auto Maxi(int32x4_t a, int32x4_t b) { return vmaxq_s32(a, b); }

RSIMD_FORCEINLINE auto Andi(int32x4_t a, int32x4_t b) { return vandq_s32(a, b); }
RSIMD_FORCEINLINE auto Ori(int32x4_t a, int32x4_t b) { return vorrq_s32(a, b); }
RSIMD_FORCEINLINE auto Xori(int32x4_t a, int32x4_t b) { return veorq_s32(a, b); }
RSIMD_FORCEINLINE auto AndNoti(int32x4_t a, int32x4_t b) {
    return vbicq_s32(b, a);
}

RSIMD_FORCEINLINE auto Blendvi(int32x4_t a, int32x4_t b, uint32x4_t mask) {
    return vbslq_s32(mask, b, a);
}
RSIMD_FORCEINLINE auto Selecti(uint32x4_t mask, int32x4_t a, int32x4_t b) {
    return vbslq_s32(mask, a, b);
}

RSIMD_FORCEINLINE int32_t HAddi(int32x4_t vec) {
    int32x2_t vlow = vget_low_s32(vec);
    int32x2_t vhigh = vget_high_s32(vec);
    vlow = vadd_s32(vlow, vhigh);
    vlow = vpadd_s32(vlow, vlow);
    return vget_lane_s32(vlow, 0);
}
RSIMD_FORCEINLINE int32_t HMini(int32x4_t vec) {
    int32x2_t vlow = vget_low_s32(vec);
    int32x2_t vhigh = vget_high_s32(vec);
    vlow = vmin_s32(vlow, vhigh);
    vlow = vpmin_s32(vlow, vlow);
    return vget_lane_s32(vlow, 0);
}
RSIMD_FORCEINLINE int32_t HMaxi(int32x4_t vec) {
    int32x2_t vlow = vget_low_s32(vec);
    int32x2_t vhigh = vget_high_s32(vec);
    vlow = vmax_s32(vlow, vhigh);
    vlow = vpmax_s32(vlow, vlow);
    return vget_lane_s32(vlow, 0);
}

RSIMD_FORCEINLINE auto Shufflei(int32x4_t a, int32x4_t b, int imm8) {
    int32x4_t result;
    result = vsetq_lane_s32(vgetq_lane_s32((imm8 & 1) ? b : a, (imm8 >> 0) & 1), result, 0);
    result = vsetq_lane_s32(vgetq_lane_s32((imm8 & 2) ? b : a, (imm8 >> 1) & 1), result, 1);
    result = vsetq_lane_s32(vgetq_lane_s32((imm8 & 4) ? b : a, (imm8 >> 2) & 1), result, 2);
    result = vsetq_lane_s32(vgetq_lane_s32((imm8 & 8) ? b : a, (imm8 >> 3) & 1), result, 3);
    return result;
}
RSIMD_FORCEINLINE auto Permutei(int32x4_t a, int32x4_t, int) { return a; }
RSIMD_FORCEINLINE auto BroadcastLanei(int32x4_t a, int lane) {
    int32x2_t half = (lane < 2) ? vget_low_s32(a) : vget_high_s32(a);
    return vdupq_lane_s32(half, lane & 1);
}

RSIMD_FORCEINLINE auto ConvertToFloati(int32x4_t a) { return vcvtq_f32_s32(a); }
RSIMD_FORCEINLINE auto ConvertToInti(float32x4_t a) { return vcvtq_s32_f32(a); }

#endif

// =========================================================================
// 泛型分发层 (NEON)
// =========================================================================

#if defined(RSIMD_NEON)

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

template <typename T> RSIMD_FORCEINLINE auto FMAdd(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FMAddf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return Addd(Muld(a, b), c);
    else if constexpr (std::is_same_v<T, int32_t>) return Addi(Muli(a, b), c);
}
template <typename T> RSIMD_FORCEINLINE auto FMSub(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FMSubf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return Subd(Muld(a, b), c);
    else if constexpr (std::is_same_v<T, int32_t>) return Subi(Muli(a, b), c);
}
template <typename T> RSIMD_FORCEINLINE auto FNMAdd(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FNMAddf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return Subd(c, Muld(a, b));
    else if constexpr (std::is_same_v<T, int32_t>) return Subi(c, Muli(a, b));
}
template <typename T> RSIMD_FORCEINLINE auto FNMSub(auto a, auto b, auto c) {
    if constexpr (std::is_same_v<T, float>) return FNMSubf(a, b, c);
    else if constexpr (std::is_same_v<T, double>) return Negd(Addd(Muld(a, b), c));
    else if constexpr (std::is_same_v<T, int32_t>) return Negi(Addi(Muli(a, b), c));
}

template <typename T> RSIMD_FORCEINLINE auto CmpEQ(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpEQf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpEQd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return CmpEQi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto CmpNE(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpNEf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpNEd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return vmvnq_u32(vceqq_s32(a, b));
}
template <typename T> RSIMD_FORCEINLINE auto CmpLT(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpLTf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpLTd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return CmpLTi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto CmpLE(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpLEf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpLEd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return vorrq_u32(vcltq_s32(a, b), vceqq_s32(a, b));
}
template <typename T> RSIMD_FORCEINLINE auto CmpGT(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpGTf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpGTd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return CmpGTi(a, b);
}
template <typename T> RSIMD_FORCEINLINE auto CmpGE(auto a, auto b) {
    if constexpr (std::is_same_v<T, float>) return CmpGEf(a, b);
    else if constexpr (std::is_same_v<T, double>) return CmpGEd(a, b);
    else if constexpr (std::is_same_v<T, int32_t>) return vorrq_u32(vcgtq_s32(a, b), vceqq_s32(a, b));
}

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

template <typename T> RSIMD_FORCEINLINE auto Shuffle(auto a, auto b, int imm8) {
    if constexpr (std::is_same_v<T, float>) return Shufflef(a, b, imm8);
    else if constexpr (std::is_same_v<T, double>) return Shuffled(a, b, imm8);
    else if constexpr (std::is_same_v<T, int32_t>) return Shufflei(a, b, imm8);
}
template <typename T> RSIMD_FORCEINLINE auto Permute(auto a, auto b, int imm8) {
    if constexpr (std::is_same_v<T, float>) return Permutef(a, b, imm8);
    else if constexpr (std::is_same_v<T, double>) return Permuted(a, b, imm8);
    else if constexpr (std::is_same_v<T, int32_t>) return Permutei(a, b, imm8);
}
template <typename T> RSIMD_FORCEINLINE auto BroadcastLane(auto a, int lane) {
    if constexpr (std::is_same_v<T, float>) return BroadcastLanef(a, lane);
    else if constexpr (std::is_same_v<T, double>) return BroadcastLanef(a, lane);
    else if constexpr (std::is_same_v<T, int32_t>) return BroadcastLanei(a, lane);
}

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

#endif

} // namespace RandomEngine::Platform::SIMD