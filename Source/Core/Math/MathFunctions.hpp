#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <ostream>

#include "Vec.hpp"
#include "Mat.hpp"

namespace RandEngine::Core::Math{
// ===================== 标量辅助函数 =====================

// 标量 Clamp
template <Detail::NumericVec T, Detail::NumericVec U, Detail::NumericVec V>
constexpr auto Clamp(T value, U min, V max)
{
    using R = std::common_type_t<T, U, V>;
    R v = static_cast<R>(value);
    R lo = static_cast<R>(min);
    R hi = static_cast<R>(max);
    return v < lo ? lo : (v > hi ? hi : v);
}

// 标量 Lerp
template <Detail::NumericVec T, Detail::NumericVec U, Detail::NumericVec V>
constexpr auto Lerp(T a, U b, V t)
{
    using R = std::common_type_t<T, U, V>;
    return static_cast<R>(a) + (static_cast<R>(b) - static_cast<R>(a)) * static_cast<R>(t);
}

// 标量 SmoothStep
template <Detail::NumericVec T, Detail::NumericVec U, Detail::NumericVec V>
constexpr auto SmoothStep(T edge0, U edge1, V x)
{
    using R = std::common_type_t<T, U, V>;
    R t = Clamp((static_cast<R>(x) - static_cast<R>(edge0)) / (static_cast<R>(edge1) - static_cast<R>(edge0)),
                static_cast<R>(0), static_cast<R>(1));
    return t * t * (static_cast<R>(3) - static_cast<R>(2) * t);
}

// 阶跃函数
template <Detail::NumericVec T, Detail::NumericVec U>
constexpr auto Step(T edge, U x)
{
    using R = std::common_type_t<T, U>;
    return (x < edge) ? static_cast<R>(0) : static_cast<R>(1);
}

// 重映射
template <Detail::NumericVec T, Detail::NumericVec U, Detail::NumericVec V>
constexpr auto Remap(T value, U in_min, U in_max, V out_min, V out_max)
{
    using R = std::common_type_t<T, U, V>;
    double t = (static_cast<double>(value) - static_cast<double>(in_min)) /
               (static_cast<double>(in_max) - static_cast<double>(in_min));
    return static_cast<R>(static_cast<double>(out_min) +
                          t * (static_cast<double>(out_max) - static_cast<double>(out_min)));
}

// 角度单位换算
template <Detail::NumericVec T>
constexpr auto DegToRad(T degrees)
{
    return degrees * static_cast<T>(std::numbers::pi / 180.0);
}

template <Detail::NumericVec T>
constexpr auto RadToDeg(T radians)
{
    return radians * static_cast<T>(180.0 / std::numbers::pi);
}

// 浮点近似相等
template <Detail::NumericVec T, Detail::NumericVec U>
constexpr bool ApproxEqual(T a, U b, double eps = 1e-6)
{
    return std::abs(static_cast<double>(a) - static_cast<double>(b)) <= eps;
}

// ===================== Vec 计算函数 =====================

// 模长平方 (复用 Dot 的 SIMD 路径)
template <Detail::NumericVec T, std::size_t N>
T LengthSquared(const Vec<T, N> &v)
{
    return Dot(v, v);
}

// 模长
template <Detail::NumericVec T, std::size_t N>
T Length(const Vec<T, N> &v)
{
    return static_cast<T>(std::sqrt(LengthSquared(v)));
}

// 归一化
template <Detail::NumericVec T, std::size_t N>
Vec<T, N> Normalize(const Vec<T, N> &v)
{
    T len = Length(v);
    if (len > 0)
    {
        return (v) / len;
    }
    return Vec<T, N>{};
}

// 点积
template <typename... Vecs>
    requires(sizeof...(Vecs) >= 2)
auto Dot(const Vecs &...vecs)
{
    constexpr std::size_t N = (std::tuple_element_t<0, std::tuple<Vecs...>>::Size());
    static_assert(((vecs.Size() == N) && ...), "All vectors must have the same dimension N");
    using ResultType = std::common_type_t<typename Vecs::vec_type_alias...>;

    if constexpr (sizeof...(Vecs) == 2 &&
                  (std::is_same_v<typename std::remove_cvref_t<Vecs>::vec_type_alias, ResultType> && ...) &&
                  Detail::VecUseSIMD<ResultType, N>)
    {
        constexpr std::size_t W = simd::SIMDWidth<ResultType>;
        auto sum_vec = simd::zero<ResultType>();
        std::size_t i = 0;
        auto tuple_vecs = std::forward_as_tuple(vecs...);
        const auto &v1 = std::get<0>(tuple_vecs);
        const auto &v2 = std::get<1>(tuple_vecs);
        for (; i + W <= N; i += W)
        {
            auto va = simd::loadu<ResultType>(&v1[i]);
            auto vb = simd::loadu<ResultType>(&v2[i]);
            sum_vec = simd::fmadd<ResultType>(va, vb, sum_vec);
        }
        ResultType total_sum = simd::hadd<ResultType>(sum_vec);
        for (; i < N; ++i)
        {
            total_sum += static_cast<ResultType>(v1[i]) * static_cast<ResultType>(v2[i]);
        }
        return total_sum;
    }
    else
    {
        ResultType total_sum = 0;
        for (std::size_t i = 0; i < N; ++i)
        {
            total_sum += (static_cast<ResultType>(vecs[i]) * ...);
        }
        return total_sum;
    }
}

// 叉积
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr auto Cross(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
    requires(N == 2 || N == 3 || N == 7)
{
    using ResultType = std::common_type_t<T, U>;

    if constexpr (N == 2) {
        return static_cast<ResultType>(lhs[0]) * rhs[1] - static_cast<ResultType>(lhs[1]) * rhs[0];
    }
    else if constexpr (N == 3) {
        return Vec<ResultType, 3>{
            static_cast<ResultType>(lhs[1]) * rhs[2] - static_cast<ResultType>(lhs[2]) * rhs[1],
            static_cast<ResultType>(lhs[2]) * rhs[0] - static_cast<ResultType>(lhs[0]) * rhs[2],
            static_cast<ResultType>(lhs[0]) * rhs[1] - static_cast<ResultType>(lhs[1]) * rhs[0]
        };
    }
    else if constexpr (N == 7) {
        Vec<ResultType, 7> res;
        res[0] = lhs[1]*rhs[3] - lhs[3]*rhs[1] + lhs[2]*rhs[6] - lhs[6]*rhs[2] + lhs[4]*rhs[5] - lhs[5]*rhs[4];
        res[1] = lhs[2]*rhs[4] - lhs[4]*rhs[2] + lhs[3]*rhs[0] - lhs[0]*rhs[3] + lhs[5]*rhs[6] - lhs[6]*rhs[5];
        res[2] = lhs[3]*rhs[5] - lhs[5]*rhs[3] + lhs[4]*rhs[1] - lhs[1]*rhs[4] + lhs[6]*rhs[0] - lhs[0]*rhs[6];
        res[3] = lhs[4]*rhs[6] - lhs[6]*rhs[4] + lhs[5]*rhs[2] - lhs[2]*rhs[5] + lhs[0]*rhs[1] - lhs[1]*rhs[0];
        res[4] = lhs[5]*rhs[0] - lhs[0]*rhs[5] + lhs[6]*rhs[3] - lhs[3]*rhs[6] + lhs[1]*rhs[2] - lhs[2]*rhs[1];
        res[5] = lhs[6]*rhs[1] - lhs[1]*rhs[6] + lhs[0]*rhs[4] - lhs[4]*rhs[0] + lhs[2]*rhs[3] - lhs[3]*rhs[2];
        res[6] = lhs[0]*rhs[2] - lhs[2]*rhs[0] + lhs[1]*rhs[5] - lhs[5]*rhs[1] + lhs[3]*rhs[4] - lhs[4]*rhs[3];
        return res;
    }
}

// 叉积运算符
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr auto operator^(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
    requires(N == 2 || N == 3 || N == 7)
{
    return Cross(lhs, rhs);
}

// 向量 Hadamard 积 (按元素相乘)
template <typename... Args>
    requires(sizeof...(Args) >= 2) &&
            (... && requires { typename std::remove_cvref_t<Args>::vec_type_alias; })
constexpr auto Hadamard(const Args &...args)
{

    using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;
    constexpr size_t N = std::remove_cvref_t<FirstArg>::Size();

    static_assert((... && (Args::Size() == N)),
                  "All vectors must have the same dimension for Hadamard product.");

    using ResultScalar = std::common_type_t<typename std::remove_cvref_t<Args>::vec_type_alias...>;

    Vec<ResultScalar, N> result;

    for (size_t i = 0; i < N; ++i)
    {
        result[i] = (static_cast<ResultScalar>(args[i]) * ...);
    }

    return result;
}

// 拼接
template <typename... Vecs>
    requires(sizeof...(Vecs) >= 1)
auto Cat(const Vecs &...vecs)
{
    constexpr std::size_t TotalN = (Vecs::Size() + ...);
    using ResultT = std::common_type_t<typename Vecs::vec_type_alias...>;
    Vec<ResultT, TotalN> result;
    std::size_t offset = 0;
    ([&](const auto &v)
     {
        for (std::size_t i = 0; i < v.Size(); ++i) {
            result[offset++] = static_cast<ResultT>(v[i]);
        } }(vecs), ...);

    return result;
}

// 距离平方
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
auto DistanceSquared(const Vec<T, N> &a, const Vec<U, N> &b)
{
    using CalcT = std::common_type_t<T, U>;
    Vec<CalcT, N> diff;
    for (size_t i = 0; i < N; ++i)
    {
        diff[i] = static_cast<CalcT>(a[i]) - static_cast<CalcT>(b[i]);
    }
    return Dot(diff, diff);
}

// 计算距离
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
auto Distance(const Vec<T, N> &a, const Vec<U, N> &b)
{
    return std::sqrt(DistanceSquared(a, b));
}

// 线性插值
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N, typename V>
auto Lerp(const Vec<T, N> &a, const Vec<U, N> &b, V t)
{
    using ResultT = std::common_type_t<T, U, V>;
    return Vec<ResultT, N>(a) * (static_cast<ResultT>(1.0) - static_cast<ResultT>(t)) + Vec<ResultT, N>(b) * static_cast<ResultT>(t);
}

// 投影 Project
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
auto Project(const Vec<T, N> &a, const Vec<U, N> &b)
{
    using ResultT = std::common_type_t<T, U>;
    auto dot_val = Dot(a, b);
    auto b_mag_sq = Dot(b, b);
    return Vec<ResultT, N>(b) * (static_cast<ResultT>(dot_val) / static_cast<ResultT>(b_mag_sq));
}

// 反射 Reflect
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
auto Reflect(const Vec<T, N> &a, const Vec<U, N> &n)
{
    using ResultT = std::common_type_t<T, U>;
    auto dot_an = Dot(a, n);
    auto dot_nn = Dot(n, n);
    return Vec<ResultT, N>(a) - Vec<ResultT, N>(n) * (static_cast<ResultT>(2) * static_cast<ResultT>(dot_an) / static_cast<ResultT>(dot_nn));
}

// 计算两个向量之间的弧度
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr double Radian(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
{
    double dot = static_cast<double>(Dot(lhs , rhs));
    double len_product = Length(lhs) * Length(rhs);

    if (len_product < 1e-9)
        return 0.0;

    double cos_theta = dot / len_product;

    if (cos_theta > 1.0)
        cos_theta = 1.0;
    if (cos_theta < -1.0)
        cos_theta = -1.0;

    return std::acos(cos_theta);
}

// 计算两个向量之间的角度 
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr double Degree(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
{
    return Radian(lhs, rhs) * (180.0 / std::numbers::pi);
}

// ===================== Vec 扩展函数 =====================

// 分量绝对值
template <Detail::NumericVec T, std::size_t N>
constexpr Vec<T, N> Abs(const Vec<T, N> &v)
    requires(!std::is_unsigned_v<T>)
{
    Vec<T, N> result;
    for (size_t i = 0; i < N; ++i)
    {
        result[i] = static_cast<T>(std::abs(v[i]));
    }
    return result;
}

// 分量最小值
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr auto Min(const Vec<T, N> &a, const Vec<U, N> &b)
{
    using R = std::common_type_t<T, U>;
    Vec<R, N> result;
    for (size_t i = 0; i < N; ++i)
    {
        R av = static_cast<R>(a[i]);
        R bv = static_cast<R>(b[i]);
        result[i] = (av < bv) ? av : bv;
    }
    return result;
}

// 分量最大值
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr auto Max(const Vec<T, N> &a, const Vec<U, N> &b)
{
    using R = std::common_type_t<T, U>;
    Vec<R, N> result;
    for (size_t i = 0; i < N; ++i)
    {
        R av = static_cast<R>(a[i]);
        R bv = static_cast<R>(b[i]);
        result[i] = (av > bv) ? av : bv;
    }
    return result;
}

// 分量级 Clamp (向量范围)
template <Detail::NumericVec T, Detail::NumericVec U, Detail::NumericVec V, std::size_t N>
constexpr auto Clamp(const Vec<T, N> &v, const Vec<U, N> &min, const Vec<V, N> &max)
{
    using R = std::common_type_t<T, U, V>;
    Vec<R, N> result;
    for (size_t i = 0; i < N; ++i)
    {
        R val = static_cast<R>(v[i]);
        R lo = static_cast<R>(min[i]);
        R hi = static_cast<R>(max[i]);
        result[i] = val < lo ? lo : (val > hi ? hi : val);
    }
    return result;
}

// 分量级 Clamp (标量范围)
template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U, Detail::NumericVec V>
constexpr auto Clamp(const Vec<T, N> &v, U min, V max)
{
    using R = std::common_type_t<T, U, V>;
    Vec<R, N> result;
    R lo = static_cast<R>(min);
    R hi = static_cast<R>(max);
    for (size_t i = 0; i < N; ++i)
    {
        R val = static_cast<R>(v[i]);
        result[i] = val < lo ? lo : (val > hi ? hi : val);
    }
    return result;
}

// 模长限制
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr auto ClampMagnitude(const Vec<T, N> &v, U max_len)
{
    using R = std::common_type_t<T, U>;
    R len_sq = static_cast<R>(LengthSquared(v));
    R max_sq = static_cast<R>(max_len) * static_cast<R>(max_len);
    if (len_sq > max_sq)
    {
        R scale = static_cast<R>(max_len) / std::sqrt(len_sq);
        return Vec<R, N>(v) * scale;
    }
    return Vec<R, N>(v);
}

// 求和
template <Detail::NumericVec T, std::size_t N>
constexpr T Sum(const Vec<T, N> &v)
{
    T s = 0;
    for (size_t i = 0; i < N; ++i)
        s += v[i];
    return s;
}

// 均值
template <Detail::NumericVec T, std::size_t N>
constexpr auto Mean(const Vec<T, N> &v)
{
    return Sum(v) / static_cast<T>(N);
}

// 最大/最小分量值
template <Detail::NumericVec T, std::size_t N>
constexpr T MaxComponent(const Vec<T, N> &v)
{
    T m = v[0];
    for (size_t i = 1; i < N; ++i)
        if (v[i] > m) m = v[i];
    return m;
}

template <Detail::NumericVec T, std::size_t N>
constexpr T MinComponent(const Vec<T, N> &v)
{
    T m = v[0];
    for (size_t i = 1; i < N; ++i)
        if (v[i] < m) m = v[i];
    return m;
}

// 最大/最小分量索引
template <Detail::NumericVec T, std::size_t N>
constexpr std::size_t MaxIndex(const Vec<T, N> &v)
{
    std::size_t idx = 0;
    for (size_t i = 1; i < N; ++i)
        if (v[i] > v[idx]) idx = i;
    return idx;
}

template <Detail::NumericVec T, std::size_t N>
constexpr std::size_t MinIndex(const Vec<T, N> &v)
{
    std::size_t idx = 0;
    for (size_t i = 1; i < N; ++i)
        if (v[i] < v[idx]) idx = i;
    return idx;
}

// 归一化线性插值
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N, typename V>
auto Nlerp(const Vec<T, N> &a, const Vec<U, N> &b, V t)
{
    return Normalize(Lerp(a, b, t));
}

// 球面插值 (近共线时退化为 Lerp)
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N, typename V>
auto Slerp(const Vec<T, N> &a, const Vec<U, N> &b, V t)
{
    using R = std::common_type_t<T, U, V>;
    Vec<R, N> va(a);
    Vec<R, N> vb(b);
    R dot = Clamp(static_cast<R>(Dot(va, vb)), static_cast<R>(-1), static_cast<R>(1));
    if (dot > static_cast<R>(0.9995) || dot < static_cast<R>(-0.9995))
        return Lerp(va, vb, t);
    R theta = std::acos(dot);
    R sin_theta = std::sin(theta);
    R one = static_cast<R>(1);
    R w1 = std::sin((one - static_cast<R>(t)) * theta) / sin_theta;
    R w2 = std::sin(static_cast<R>(t) * theta) / sin_theta;
    return va * w1 + vb * w2;
}

// 逆插值 (分量级)
template <Detail::NumericVec T, Detail::NumericVec U, Detail::NumericVec V, std::size_t N>
constexpr auto InverseLerp(const Vec<T, N> &a, const Vec<U, N> &b, const Vec<V, N> &v)
{
    using R = std::common_type_t<T, U, V>;
    Vec<R, N> result;
    for (size_t i = 0; i < N; ++i)
    {
        R denom = static_cast<R>(b[i]) - static_cast<R>(a[i]);
        result[i] = (std::abs(denom) < static_cast<R>(1e-9))
                        ? static_cast<R>(0)
                        : (static_cast<R>(v[i]) - static_cast<R>(a[i])) / denom;
    }
    return result;
}

// 平滑插值 (向量)
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N, typename V>
constexpr auto SmoothStep(const Vec<T, N> &edge0, const Vec<U, N> &edge1, V x)
{
    using R = std::common_type_t<T, U, V>;
    Vec<R, N> result;
    R xv = static_cast<R>(x);
    for (size_t i = 0; i < N; ++i)
    {
        R e0 = static_cast<R>(edge0[i]);
        R e1 = static_cast<R>(edge1[i]);
        R t = Clamp((xv - e0) / (e1 - e0), static_cast<R>(0), static_cast<R>(1));
        result[i] = t * t * (static_cast<R>(3) - static_cast<R>(2) * t);
    }
    return result;
}

// 外积 a ⊗ b → Mat<N,N>
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr auto OuterProduct(const Vec<T, N> &a, const Vec<U, N> &b)
{
    using R = std::common_type_t<T, U>;
    Mat<R, N, N> result;
    for (size_t r = 0; r < N; ++r)
        for (size_t c = 0; c < N; ++c)
            result[r, c] = static_cast<R>(a[r]) * static_cast<R>(b[c]);
    return result;
}

// 标量三重积 a · (b × c)
template <Detail::NumericVec T, Detail::NumericVec U, Detail::NumericVec V, std::size_t N>
constexpr auto TripleProduct(const Vec<T, N> &a, const Vec<U, N> &b, const Vec<V, N> &c)
    requires(N == 3)
{
    return Dot(a, Cross(b, c));
}

// 投影到平面
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
auto ProjectOnPlane(const Vec<T, N> &v, const Vec<U, N> &normal)
{
    using R = std::common_type_t<T, U>;
    return Vec<R, N>(v) - Project(v, normal);
}

// 折射
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N, typename V>
auto Refract(const Vec<T, N> &I, const Vec<U, N> &n, V eta)
{
    using R = std::common_type_t<T, U, V>;
    Vec<R, N> ni(I);
    Vec<R, N> nn(n);
    R dot = static_cast<R>(Dot(ni, nn));
    R k = static_cast<R>(1) - static_cast<R>(eta) * static_cast<R>(eta) * (static_cast<R>(1) - dot * dot);
    if (k < static_cast<R>(0))
        return Vec<R, N>{};
    return ni * static_cast<R>(eta) - nn * (static_cast<R>(eta) * dot + std::sqrt(k));
}

// 面法线
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
auto Faceforward(const Vec<T, N> &n, const Vec<U, N> &I, const Vec<U, N> &Nref)
{
    return (Dot(Nref, I) < 0) ? n : -n;
}

// 平行判定 (尺度无关)
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr bool IsParallel(const Vec<T, N> &a, const Vec<U, N> &b)
    requires(N == 2 || N == 3 || N == 7)
{
    double len_a = static_cast<double>(LengthSquared(a));
    double len_b = static_cast<double>(LengthSquared(b));
    if (len_a < 1e-18 || len_b < 1e-18)
        return true;
    double cross_sq;
    if constexpr (N == 2)
        cross_sq = static_cast<double>(Cross(a, b)) * static_cast<double>(Cross(a, b));
    else
        cross_sq = static_cast<double>(LengthSquared(Cross(a, b)));
    return cross_sq <= 1e-12 * len_a * len_b;
}

// 正交判定 (尺度无关)
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr bool IsOrthogonal(const Vec<T, N> &a, const Vec<U, N> &b)
{
    double dot = static_cast<double>(Dot(a, b));
    double len_a = static_cast<double>(LengthSquared(a));
    double len_b = static_cast<double>(LengthSquared(b));
    if (len_a < 1e-18 || len_b < 1e-18)
        return true;
    return (dot * dot) <= 1e-12 * len_a * len_b;
}

// 中点
template <Detail::NumericVec T, Detail::NumericVec U, std::size_t N>
constexpr auto Midpoint(const Vec<T, N> &a, const Vec<U, N> &b)
{
    using R = std::common_type_t<T, U>;
    return (Vec<R, N>(a) + Vec<R, N>(b)) / static_cast<R>(2);
}

// ===================== Mat 计算函数 =====================

// 矩阵 Hadamard 积
template <typename... Args>
    requires(sizeof...(Args) >= 2) &&
            (... && requires { typename std::remove_cvref_t<Args>::mat_type_alias; })
constexpr auto Hadamard(const Args &...args)
{
    using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;

    constexpr size_t R = std::remove_cvref_t<FirstArg>::RowSize();
    constexpr size_t C = std::remove_cvref_t<FirstArg>::ColSize();

    static_assert((... && (Args::RowSize() == R && Args::ColSize() == C)),
                  "All matrices must have the same dimensions.");

    using ResultScalar = std::common_type_t<typename std::remove_cvref_t<Args>::mat_type_alias...>;

    Mat<ResultScalar, R, C> result;

    for (size_t i = 0; i < R * C; ++i)
    {
        result[i] = (static_cast<ResultScalar>(args[i]) * ...);
    }

    return result;
}

// 矩阵克罗内积
template <Detail::NumericMat T, Detail::NumericMat U,
          size_t Row1, size_t Col1, size_t Row2, size_t Col2>
constexpr auto KroneckerProduct(const Mat<T, Row1, Col1> &lhs,
                                const Mat<U, Row2, Col2> &rhs)
{
    using ResultType = std::common_type_t<T, U>;
    constexpr size_t ResultRow = Row1 * Row2;
    constexpr size_t ResultCol = Col1 * Col2;

    Mat<ResultType, ResultRow, ResultCol> result;

    for (size_t i = 0; i < Row1; ++i)
    {
        for (size_t j = 0; j < Col1; ++j)
        {
            T scalar = lhs[i, j];
            for (size_t k = 0; k < Row2; ++k)
            {
                for (size_t l = 0; l < Col2; ++l)
                {
                    result[i * Row2 + k, j * Col2 + l] =
                        static_cast<ResultType>(scalar) *
                        static_cast<ResultType>(rhs[k, l]);
                }
            }
        }
    }
    return result;
}

template <typename T, typename... Args>
constexpr auto Kronecker(const T &first, const Args &...rest)
{
    if constexpr (sizeof...(rest) == 0)
    {
        return first;
    }
    else
    {
        return KroneckerProduct(first, Kronecker(rest...));
    }
}

// 转置
template <Detail::NumericMat T, size_t Row, size_t Col>
constexpr auto Transpose(const Mat<T, Row, Col> &mat)
{
    Mat<T, Col, Row> result;
    for (size_t r = 0; r < Row; ++r)
    {
        for (size_t c = 0; c < Col; ++c)
        {
            result[c, r] = mat[r, c];
        }
    }
    return result;
}

// 辅助函数：获取子矩阵 (用于计算余子式)
template <Detail::NumericMat T, size_t Row, size_t Col>
constexpr auto MinorMatrix(const Mat<T, Row, Col> &mat, size_t omitRow, size_t omitCol)
{
    static_assert(Row > 1 && Col > 1, "Cannot get minor of a 1x1 matrix.");
    Mat<T, Row - 1, Col - 1> result;
    size_t rr = 0;
    for (size_t r = 0; r < Row; ++r)
    {
        if (r == omitRow)
            continue;
        size_t cc = 0;
        for (size_t c = 0; c < Col; ++c)
        {
            if (c == omitCol)
                continue;
            result[rr, cc] = mat[r, c];
            cc++;
        }
        rr++;
    }
    return result;
}

// 行列式取值
template <Detail::NumericMat T, size_t Size>
constexpr auto Det(const Mat<T, Size, Size> &mat)
{
    if constexpr (Size == 1)
        return mat[0];
    if constexpr (Size == 2)
        return mat[0, 0] * mat[1, 1] - mat[0, 1] * mat[1, 0];

    auto temp = mat;
    T det = 1;

    for (size_t i = 0; i < Size; ++i)
    {
        // 寻找主元
        size_t pivot = i;
        for (size_t j = i + 1; j < Size; ++j)
        {
            if (std::abs(temp[j, i]) > std::abs(temp[pivot, i]))
                pivot = j;
        }

        if (Detail::IsNearZero(temp[pivot, i]))
            return static_cast<T>(0);

        if (pivot != i)
        {
            // 交换行，行列式变号
            for (size_t k = i; k < Size; ++k)
                std::swap(temp[i, k], temp[pivot, k]);
            det *= -1;
        }

        det *= temp[i, i];

        for (size_t j = i + 1; j < Size; ++j)
        {
            T factor = temp[j, i] / temp[i, i];
            for (size_t k = i + 1; k < Size; ++k)
            {
                temp[j, k] -= factor * temp[i, k];
            }
        }
    }
    return det;
}

// --- 代数余子式 (Cofactor) ---
template <Detail::NumericMat T, size_t Size>
constexpr auto Cofactor(const Mat<T, Size, Size> &mat, size_t row, size_t col)
{
    auto minorDet = Det(MinorMatrix(mat, row, col));
    return ((row + col) % 2 == 0) ? minorDet : -minorDet;
}

// --- 伴随矩阵 (Adjoint) ---
template <Detail::NumericMat T, size_t Size>
constexpr auto Adjoint(const Mat<T, Size, Size> &mat)
{
    if constexpr (Size == 1)
    {
        return Mat<T, 1, 1>{1};
    }
    Mat<T, Size, Size> adj;
    for (size_t r = 0; r < Size; ++r)
    {
        for (size_t c = 0; c < Size; ++c)
        {
            adj[c, r] = Cofactor(mat, r, c);
        }
    }
    return adj;
}

// 逆矩阵 (高斯-约当消元，O(n^3)，含部分主元)
template <Detail::NumericMat T, size_t Size>
constexpr auto Inverse(const Mat<T, Size, Size> &mat)
{
    Mat<T, Size, 2 * Size> aug;
    for (size_t r = 0; r < Size; ++r)
    {
        for (size_t c = 0; c < Size; ++c)
        {
            aug[r, c] = mat[r, c];
            aug[r, c + Size] = (r == c) ? static_cast<T>(1) : static_cast<T>(0);
        }
    }

    for (size_t i = 0; i < Size; ++i)
    {
        // 部分主元
        size_t pivot = i;
        for (size_t j = i + 1; j < Size; ++j)
        {
            if (std::abs(aug[j, i]) > std::abs(aug[pivot, i]))
                pivot = j;
        }

        if (Detail::IsNearZero(aug[pivot, i]))
        {
            throw std::runtime_error("Matrix is singular and cannot be inverted.");
        }

        if (pivot != i)
        {
            for (size_t k = 0; k < 2 * Size; ++k)
                std::swap(aug[i, k], aug[pivot, k]);
        }

        // 归一化主元行
        T pivot_val = aug[i, i];
        for (size_t k = 0; k < 2 * Size; ++k)
            aug[i, k] = aug[i, k] / pivot_val;

        // 消去其他行
        for (size_t j = 0; j < Size; ++j)
        {
            if (j == i)
                continue;
            T factor = aug[j, i];
            if (Detail::IsNearZero(factor))
                continue;
            for (size_t k = 0; k < 2 * Size; ++k)
                aug[j, k] -= factor * aug[i, k];
        }
    }

    Mat<T, Size, Size> result;
    for (size_t r = 0; r < Size; ++r)
        for (size_t c = 0; c < Size; ++c)
            result[r, c] = aug[r, c + Size];
    return result;
}

// 矩阵的迹
template <Detail::NumericMat T, size_t Size>
constexpr auto Trace(const Mat<T, Size, Size> &mat)
{
    T trace = 0;
    for (size_t i = 0; i < Size; ++i)
    {
        trace += mat[i, i];
    }
    return trace;
}

// 矩阵的秩
template <Detail::NumericMat T, size_t Row, size_t Col>
constexpr size_t Rank(const Mat<T, Row, Col> &mat)
{
    auto temp = mat;
    size_t rank = 0;
    std::vector<bool> row_used(Row, false);

    for (size_t i = 0; i < Col && rank < Row; ++i)
    {
        size_t pivot = Row;
        for (size_t j = 0; j < Row; ++j)
        {
            if (!row_used[j] && !Detail::IsNearZero(temp[j, i]))
            {
                pivot = j;
                break;
            }
        }

        if (pivot != Row)
        {
            row_used[pivot] = true;
            rank++;
            for (size_t j = 0; j < Row; ++j)
            {
                if (!row_used[j])
                {
                    T factor = temp[j, i] / temp[pivot, i];
                    for (size_t k = i; k < Col; ++k)
                    {
                        temp[j, k] -= factor * temp[pivot, k];
                    }
                }
            }
        }
    }
    return rank;
}

// 满秩判断
template <Detail::NumericMat T, size_t Size>
constexpr bool IsFullRank(const Mat<T, Size, Size> &mat)
{
    return Rank(mat) == Size;
}

// ===================== Mat 扩展函数 =====================

// 对角矩阵
template <Detail::NumericVec T, std::size_t N>
constexpr Mat<T, N, N> MakeDiagonal(const Vec<T, N> &v)
{
    Mat<T, N, N> result;
    for (size_t i = 0; i < N; ++i)
        result[i, i] = v[i];
    return result;
}

// 提取对角线
template <Detail::NumericMat T, size_t Size>
constexpr Vec<T, Size> Diagonal(const Mat<T, Size, Size> &m)
{
    Vec<T, Size> result;
    for (size_t i = 0; i < Size; ++i)
        result[i] = m[i, i];
    return result;
}

// 单位矩阵判定
template <Detail::NumericMat T, size_t Size>
constexpr bool IsIdentity(const Mat<T, Size, Size> &m)
{
    for (size_t r = 0; r < Size; ++r)
        for (size_t c = 0; c < Size; ++c)
        {
            T expected = (r == c) ? static_cast<T>(1) : static_cast<T>(0);
            if (!Detail::IsNearZero(m[r, c] - expected))
                return false;
        }
    return true;
}

// 对角矩阵判定
template <Detail::NumericMat T, size_t Row, size_t Col>
constexpr bool IsDiagonal(const Mat<T, Row, Col> &m)
{
    for (size_t r = 0; r < Row; ++r)
        for (size_t c = 0; c < Col; ++c)
            if (r != c && !Detail::IsNearZero(m[r, c]))
                return false;
    return true;
}

// 对称矩阵判定
template <Detail::NumericMat T, size_t Size>
constexpr bool IsSymmetric(const Mat<T, Size, Size> &m)
{
    for (size_t r = 0; r < Size; ++r)
        for (size_t c = 0; c < Size; ++c)
            if (!Detail::IsNearZero(m[r, c] - m[c, r]))
                return false;
    return true;
}

// 反对称矩阵判定
template <Detail::NumericMat T, size_t Size>
constexpr bool IsSkewSymmetric(const Mat<T, Size, Size> &m)
{
    for (size_t r = 0; r < Size; ++r)
        for (size_t c = 0; c < Size; ++c)
            if (!Detail::IsNearZero(m[r, c] + m[c, r]))
                return false;
    return true;
}

// 正交矩阵判定 (M * M^T = I)
template <Detail::NumericMat T, size_t Size>
constexpr bool IsOrthogonal(const Mat<T, Size, Size> &m)
{
    auto t = m * Transpose(m);
    for (size_t r = 0; r < Size; ++r)
        for (size_t c = 0; c < Size; ++c)
        {
            T expected = (r == c) ? static_cast<T>(1) : static_cast<T>(0);
            if (!Detail::IsNearZero(t[r, c] - expected))
                return false;
        }
    return true;
}

// 奇异矩阵判定
template <Detail::NumericMat T, size_t Size>
constexpr bool IsSingular(const Mat<T, Size, Size> &m)
{
    return Detail::IsNearZero(Det(m));
}

// 弗罗贝尼乌斯范数
template <Detail::NumericMat T, size_t Row, size_t Col>
constexpr T FrobeniusNorm(const Mat<T, Row, Col> &m)
{
    T sum = 0;
    for (size_t i = 0; i < Row * Col; ++i)
        sum += m[i] * m[i];
    return std::sqrt(sum);
}

// 解线性方程组 Ax = b (高斯消元 + 部分主元)
template <Detail::NumericMat T, size_t Size>
constexpr Vec<T, Size> SolveLinearSystem(const Mat<T, Size, Size> &A, const Vec<T, Size> &b)
{
    Mat<T, Size, Size + 1> aug;
    for (size_t r = 0; r < Size; ++r)
    {
        for (size_t c = 0; c < Size; ++c)
            aug[r, c] = A[r, c];
        aug[r, Size] = b[r];
    }

    for (size_t i = 0; i < Size; ++i)
    {
        size_t pivot = i;
        for (size_t j = i + 1; j < Size; ++j)
            if (std::abs(aug[j, i]) > std::abs(aug[pivot, i]))
                pivot = j;

        if (Detail::IsNearZero(aug[pivot, i]))
            throw std::runtime_error("Linear system is singular or has no unique solution.");

        if (pivot != i)
            for (size_t k = i; k <= Size; ++k)
                std::swap(aug[i, k], aug[pivot, k]);

        T pv = aug[i, i];
        for (size_t j = i + 1; j < Size; ++j)
        {
            T factor = aug[j, i] / pv;
            for (size_t k = i; k <= Size; ++k)
                aug[j, k] -= factor * aug[i, k];
        }
    }

    // 回代
    Vec<T, Size> x;
    for (int i = static_cast<int>(Size) - 1; i >= 0; --i)
    {
        T sum = aug[i, Size];
        for (size_t j = static_cast<size_t>(i) + 1; j < Size; ++j)
            sum -= aug[i, j] * x[j];
        x[i] = sum / aug[i, i];
    }
    return x;
}

// 矩阵幂 (n >= 0, 快速幂)
template <Detail::NumericMat T, size_t Size>
constexpr Mat<T, Size, Size> MatrixPower(const Mat<T, Size, Size> &m, int n)
{
    if (n < 0)
        throw std::runtime_error("MatrixPower: negative exponent is not supported.");
    Mat<T, Size, Size> result = Mat<T, Size, Size>::MakeIdentity();
    Mat<T, Size, Size> base = m;
    while (n > 0)
    {
        if (n & 1)
            result = result * base;
        base = base * base;
        n >>= 1;
    }
    return result;
}

// 摩尔-彭罗斯伪逆 (满秩方阵: (A^T A)^{-1} A^T)
template <Detail::NumericMat T, size_t Size>
constexpr auto PseudoInverse(const Mat<T, Size, Size> &m)
{
    auto mt = Transpose(m);
    auto a = mt * m;
    return Inverse(a) * mt;
}

// ===================== 输出运算符 =====================

// Vec 输出运算符
template <Detail::NumericVec T, std::size_t N>
std::ostream &operator<<(std::ostream &os, const Vec<T, N> &v)
{
    os << "[";
    for (size_t i = 0; i < N; ++i)
    {
        os << v[i];
        if (i + 1 < N)
            os << ", ";
    }
    os << "]";
    return os;
}

// VecView 输出运算符
template <Detail::NumericVec T, std::size_t N, int start, int end, int step>
constexpr std::ostream &operator<<(std::ostream &os, const VecView<T, N, start, end, step> &view)
{
    os << Vec<T, Range<start, end, step>::Size()>(view);
    return os;
}

// Mat 输出运算符
template <Detail::NumericMat T, size_t Row, size_t Col>
std::ostream &operator<<(std::ostream &os, const Mat<T, Row, Col> &mat)
{
    for (size_t r = 0; r < Row; ++r)
    {
        os << "[";
        for (size_t c = 0; c < Col; ++c)
        {
            if (mat[r, c] >= 0)
            {
                os << " ";
            }
            os << mat[r, c];
            if (c + 1 < Col)
            {
                os << ", ";
            }
        }
        os << "]";
        if (r + 1 < Row)
            os << "\n";
    }

    return os;
}

// MatView 输出运算符
template <Detail::NumericMat T, size_t Row, size_t Col, typename RowRange, typename ColRange>
std::ostream &operator<<(std::ostream &os, const MatView<T, Row, Col, RowRange, ColRange> &mat_view)
{
    os << Mat<T, RowRange::Size(), ColRange::Size()>(mat_view);
    return os;
}

}