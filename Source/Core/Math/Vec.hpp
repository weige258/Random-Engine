#pragma once

#include <array>
#include <list>
#include <vector>
#include <span>
#include <concepts>
#include <cmath>
#include <cassert>
#include <numbers>
#include "Range.hpp"
#include <stdexcept>
#include "../../Platform/SIMD/SIMD.hpp"

namespace RandomEngine::Core::Math{

namespace Detail
{
    // 概念：Numeric 表示数值类型（整数或浮点数）
    template <typename T>
    concept NumericVec = std::is_arithmetic_v<T>;

    // 结构体：CompileTimeIndexCheck 用于编译期检查索引是否超出范围
    template <std::size_t Limit>
    struct CompileTimeIndexCheckVec
    {
        std::size_t value;

        template <std::size_t I>
        consteval CompileTimeIndexCheckVec(std::integral_constant<std::size_t, I>) : value(I)
        {
            static_assert(I < Limit, "Vec index out of bounds!");
        }
    };

    // SIMD 辅助：判断两个类型是否都支持 SIMD 且结果类型也支持
    template <typename T, typename U>
    inline constexpr bool CanUseSIMD = std::is_same_v<T, U> && RandomEngine::Platform::SIMD::SupportsSIMD<T> &&
                                         RandomEngine::Platform::SIMD::SupportsSIMD<U> &&
                                         RandomEngine::Platform::SIMD::SupportsSIMD<std::common_type_t<T, U>>;

    template <typename T, std::size_t N>
    inline constexpr bool VecUseSIMD = RandomEngine::Platform::SIMD::SupportsSIMD<T> && (RandomEngine::Platform::SIMD::SIMDWidth<T> > 1) && (N >= RandomEngine::Platform::SIMD::SIMDWidth<T>);
}

// 视图声明
template <Detail::NumericVec T, std::size_t N, int start, int end, int step>
struct VecView;

// Vec 对象
template <Detail::NumericVec T, std::size_t N>
struct Vec final
{
private:
    // 数据
    std::array<T, N> m_data;

public:
    using vec_type_alias = T;
    template <Detail::NumericVec U, std::size_t M>
    friend struct Vec;

    // 标量混合运算符的友元声明
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator+(const Vec<U, M> &lhs, V rhs);
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator+(V lhs, const Vec<U, M> &rhs);
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator-(const Vec<U, M> &lhs, V rhs);
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator-(V lhs, const Vec<U, M> &rhs);
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator*(const Vec<U, M> &lhs, V rhs);
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator*(V lhs, const Vec<U, M> &rhs);
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator/(const Vec<U, M> &lhs, V rhs);
    template <Detail::NumericVec U, std::size_t M, Detail::NumericVec V>
    friend constexpr auto operator/(V lhs, const Vec<U, M> &rhs);

public:
    // 构造
    constexpr Vec() { m_data.fill(0); }

    constexpr Vec(T num) { m_data.fill(num); }

    constexpr Vec(const Vec &other) = default;

    constexpr Vec(Vec &&other) noexcept = default;

    constexpr Vec(std::initializer_list<T> list)
    {
        if (list.size() != N)
        {
            throw std::runtime_error("Initializer list Size mismatch");
        }
        std::copy(list.begin(), list.end(), m_data.begin());
    }

    template <typename... Args>
    constexpr Vec(const Args &...args)
        requires(sizeof...(args) == N && (std::convertible_to<Args, T> && ...))
    {
        m_data = std::array<T, N>{static_cast<T>(args)...};
    };

    constexpr Vec(const T *arr)
    {
        for (size_t i = 0; i < N; ++i)
            m_data[i] = arr[i];
    }

    constexpr Vec(const std::array<T, N> &array) : m_data(array) {}

    template <typename U>
    constexpr Vec(const std::list<U> &list)
        requires std::convertible_to<U, T>
    {
        if (list.size() != N)
        {
            throw std::runtime_error("List Size does not match vector dimension");
        }
        std::copy(list.begin(), list.end(), m_data.begin());
    }

    template <typename U>
    constexpr Vec(const std::vector<U> &vector)
        requires std::convertible_to<U, T>
    {
        if (vector.size() != N)
        {
            throw std::runtime_error("Vector Size does not match vector dimension");
        }
        std::copy(vector.begin(), vector.end(), m_data.begin());
    }

    constexpr Vec(std::span<const T, N> s)
    {
        std::copy(s.begin(), s.end(), m_data.begin());
    }

    static constexpr Vec<T,N> MakeZero(){
         return Vec<T,N>(static_cast<T>(0));
    }

    // 析构
    ~Vec() = default;

    // 类型转换
    template <typename U>
    constexpr Vec(const Vec<U, N> &other)
        requires std::convertible_to<U, T>
    {
        for (size_t i = 0; i < N; ++i)
        {
            m_data[i] = static_cast<T>(other.m_data[i]);
        }
    }

    template <typename U>
        requires Detail::NumericVec<U>
    operator std::array<U, N>() const
    {
        std::array<U, N> result;
        for (size_t i = 0; i < N; ++i)
        {
            result[i] = static_cast<U>(m_data[i]);
        }
        return result;
    }

    template <typename U>
        requires Detail::NumericVec<U>
    operator std::vector<U>() const
    {
        std::vector<U> result;
        result.reserve(N);
        for (const auto &val : m_data)
        {
            result.push_back(static_cast<U>(val));
        }
        return result;
    }

    template <typename U>
        requires Detail::NumericVec<U>
    operator std::list<U>() const
    {
        std::list<U> result;
        for (const auto &val : m_data)
        {
            result.push_back(static_cast<U>(val));
        }
        return result;
    }

    operator std::span<const T, N>() const
    {
        return std::span<const T, N>(m_data);
    }

    // 指针转换
    explicit operator T *() { return m_data.data(); }
    explicit operator const T *() const { return m_data.data(); }

    // 访问
    constexpr T &operator[](std::size_t index)
    {
        assert(index < N);
        return m_data[index];
    }

    constexpr const T &operator[](std::size_t index) const
    {
        assert(index < N);
        return m_data[index];
    }

    constexpr T &operator[](Detail::CompileTimeIndexCheckVec<N> index)
    {
        return m_data[index.value];
    }

    constexpr const T &operator[](Detail::CompileTimeIndexCheckVec<N> index) const
    {
        return m_data[index.value];
    }

    template <int start, int end, int step>
    constexpr auto operator[](Range<start, end, step> range)
    {
        return VecView<T, N, start, end, step>(*this, range);
    }

    template <int start, int end, int step>
    constexpr auto operator[](Range<start, end, step> range) const
    {
        constexpr std::size_t M = Range<start, end, step>::Size();
        Vec<T, M> result;
        std::size_t i = 0;
        for (int val : range)
        {
            result[i++] = m_data[(val < 0) ? static_cast<std::size_t>(static_cast<int>(N) + val) : static_cast<std::size_t>(val)];
        }
        return result;
    }

    constexpr T &X()
        requires(0 < N)
    {
        return m_data[0];
    }

    constexpr const T &X() const
        requires(0 < N)
    {
        return m_data[0];
    }

    constexpr T &Y()
        requires(1 < N)
    {
        return m_data[1];
    }

    constexpr const T &Y() const
        requires(1 < N)
    {
        return m_data[1];
    }

    constexpr T &Z()
        requires(2 < N)
    {
        return m_data[2];
    }

    constexpr const T &Z() const
        requires(2 < N)
    {
        return m_data[2];
    }

    constexpr T &W()
        requires(3 < N)
    {
        return m_data[3];
    }

    constexpr const T &W() const
        requires(3 < N)
    {
        return m_data[3];
    }

    constexpr VecView<T, N, 0, 3, 1> XYZ()
        requires(3 <= N)
    {
        return VecView<T, N, 0, 3, 1>(*this, Range<0, 3, 1>());
    }

    constexpr Vec<T, 3> XYZ() const
        requires(3 <= N)
    {
        return Vec<T, 3>{m_data[0], m_data[1], m_data[2]};
    }

    // ===================== 实例 Set 方法 =====================

    // 就地填充标量
    constexpr Vec& SetZero(){
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            auto z = RandomEngine::Platform::SIMD::Zero<T>();
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], z);
            }
            for (; i < N; ++i)
                m_data[i] = 0;
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
                m_data[i] = 0;
        }
        return *this;
    }

    constexpr Vec &SetValue(T value)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            auto v = RandomEngine::Platform::SIMD::Set1<T>(value);
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], v);
            }
            for (; i < N; ++i)
                m_data[i] = value;
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
                m_data[i] = value;
        }
        return *this;
    }

    constexpr Vec &SetX(T val)
        requires(0 < N)
    {
        m_data[0] = val;
        return *this;
    }

    constexpr Vec &SetY(T val)
        requires(1 < N)
    {
        m_data[1] = val;
        return *this;
    }

    constexpr Vec &SetZ(T val)
        requires(2 < N)
    {
        m_data[2] = val;
        return *this;
    }

    constexpr Vec &SetW(T val)
        requires(3 < N)
    {
        m_data[3] = val;
        return *this;
    }

    template <Detail::NumericVec U, Detail::NumericVec V, Detail::NumericVec W>
    constexpr Vec &SetXYZ(U x, V y, W z)
        requires(3 <= N)
    {
        m_data[0] = static_cast<T>(x);
        m_data[1] = static_cast<T>(y);
        m_data[2] = static_cast<T>(z);
        return *this;
    }

    template <Detail::NumericVec U>
    constexpr Vec &SetXYZ(const Vec<U, 3> &xyz)
        requires(3 <= N)
    {
        m_data[0] = static_cast<T>(xyz[0]);
        m_data[1] = static_cast<T>(xyz[1]);
        m_data[2] = static_cast<T>(xyz[2]);
        return *this;
    }

    template <Detail::NumericVec U, Detail::NumericVec V, Detail::NumericVec W>
    constexpr Vec &SetRGB(U r, V g, W b)
        requires(3 <= N)
    {
        return SetXYZ(r, g, b);
    }

    template <Detail::NumericVec U>
    constexpr Vec &SetRGB(const Vec<U, 3> &rgb)
        requires(3 <= N)
    {
        return SetXYZ(rgb);
    }

    constexpr VecView<T, N, 0, 3, 1> RGB()
        requires(3 <= N)
    {
        return VecView<T, N, 0, 3, 1>(*this, Range<0, 3, 1>());
    }

    constexpr Vec<T, 3> RGB() const
        requires(3 <= N)
    {
        return Vec<T, 3>{m_data[0], m_data[1], m_data[2]};
    }

    // 赋值
    constexpr Vec &operator=(const Vec &other) = default;

    constexpr Vec &operator=(Vec &&other) noexcept = default;

    constexpr Vec &operator=(const T &value)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            auto v = RandomEngine::Platform::SIMD::Set1<T>(value);
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], v);
            }
            for (; i < N; ++i)
            {
                m_data[i] = value;
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] = value;
            }
        }
        return *this;
    }

    // 运算
    template <Detail::NumericVec U>
    constexpr friend auto operator+(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Vec<ResultType, N> result;
        if constexpr (Detail::CanUseSIMD<T, U> && Detail::VecUseSIMD<ResultType, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(a, b));
            }
            for (; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) + static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) + static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        return result;
    }

    template <Detail::NumericVec U>
    constexpr friend auto operator-(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Vec<ResultType, N> result;
        if constexpr (Detail::CanUseSIMD<T, U> && Detail::VecUseSIMD<ResultType, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(a, b));
            }
            for (; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) - static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) - static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        return result;
    }

    template <Detail::NumericVec U>
    constexpr friend auto operator*(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Vec<ResultType, N> result;
        if constexpr (Detail::CanUseSIMD<T, U> && Detail::VecUseSIMD<ResultType, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Mul<ResultType>(a, b));
            }
            for (; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) * static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) * static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        return result;
    }

    template <Detail::NumericVec U>
    constexpr friend auto operator/(const Vec<T, N> &lhs, const Vec<U, N> &rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Vec<ResultType, N> result;
        if constexpr (Detail::CanUseSIMD<T, U> && Detail::VecUseSIMD<ResultType, N> && !std::is_integral_v<ResultType>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Div<ResultType>(a, b));
            }
            for (; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) / static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) / static_cast<ResultType>(rhs.m_data[i]);
            }
        }
        return result;
    }

    constexpr Vec operator-() const
    {
        Vec result{};
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            auto z = RandomEngine::Platform::SIMD::Zero<T>();
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<T>(z, a));
            }
            for (; i < N; ++i)
            {
                result[i] = -m_data[i];
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                result[i] = -m_data[i];
            }
        }
        return result;
    }

    // 复合赋值操作符
    constexpr Vec &operator+=(const Vec &other)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<T>(&other.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Add<T>(a, b));
            }
            for (; i < N; ++i)
            {
                m_data[i] += other.m_data[i];
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] += other.m_data[i];
            }
        }
        return *this;
    }

    constexpr Vec &operator+=(const T &value)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            auto sv = RandomEngine::Platform::SIMD::Set1<T>(value);
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Add<T>(a, sv));
            }
            for (; i < N; ++i)
            {
                m_data[i] += value;
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] += value;
            }
        }
        return *this;
    }

    constexpr Vec &operator-=(const Vec &other)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<T>(&other.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Sub<T>(a, b));
            }
            for (; i < N; ++i)
            {
                m_data[i] -= other.m_data[i];
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] -= other.m_data[i];
            }
        }
        return *this;
    }

    constexpr Vec &operator-=(const T &value)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            auto sv = RandomEngine::Platform::SIMD::Set1<T>(value);
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Sub<T>(a, sv));
            }
            for (; i < N; ++i)
            {
                m_data[i] -= value;
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] -= value;
            }
        }
        return *this;
    }

    constexpr Vec &operator*=(const Vec &other)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<T>(&other.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Mul<T>(a, b));
            }
            for (; i < N; ++i)
            {
                m_data[i] *= other.m_data[i];
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] *= other.m_data[i];
            }
        }
        return *this;
    }

    constexpr Vec &operator*=(const T &value)
    {
        if constexpr (Detail::VecUseSIMD<T, N>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            auto sv = RandomEngine::Platform::SIMD::Set1<T>(value);
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Mul<T>(a, sv));
            }
            for (; i < N; ++i)
            {
                m_data[i] *= value;
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] *= value;
            }
        }
        return *this;
    }

    constexpr Vec &operator/=(const Vec &other)
    {
        if constexpr (Detail::VecUseSIMD<T, N> && !std::is_integral_v<T>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                auto b = RandomEngine::Platform::SIMD::LoadU<T>(&other.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Div<T>(a, b));
            }
            for (; i < N; ++i)
            {
                m_data[i] /= other.m_data[i];
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] /= other.m_data[i];
            }
        }
        return *this;
    }

    constexpr Vec &operator/=(const T &value)
    {
        if constexpr (Detail::VecUseSIMD<T, N> && !std::is_integral_v<T>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            auto sv = RandomEngine::Platform::SIMD::Set1<T>(value);
            std::size_t i = 0;
            for (; i + W <= N; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Div<T>(a, sv));
            }
            for (; i < N; ++i)
            {
                m_data[i] /= value;
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] /= value;
            }
        }
        return *this;
    }

    constexpr Vec &operator^=(const Vec &other)
        requires(N == 3)
    {
        T x = m_data[1] * other.m_data[2] - m_data[2] * other.m_data[1];
        T y = m_data[2] * other.m_data[0] - m_data[0] * other.m_data[2];
        T z = m_data[0] * other.m_data[1] - m_data[1] * other.m_data[0];
        m_data[0] = x;
        m_data[1] = y;
        m_data[2] = z;
        return *this;
    };

    // 迭代器支持
    auto begin() noexcept { return m_data.begin(); }
    auto end() noexcept { return m_data.end(); }
    auto begin() const noexcept { return m_data.begin(); }
    auto end() const noexcept { return m_data.end(); }

    // 比较操作符 (C++20)
    auto operator<=>(const Vec &other) const = default;

    // 查询方法
    static constexpr size_t Size() noexcept { return N; }

    static constexpr size_t SizeInBytes() noexcept { return N * sizeof(T); }

    static const type_info &Type() noexcept { return typeid(Vec<T, N>); }

    static const type_info &ValueType() noexcept { return typeid(T); }
};

// VecView 定义
template <Detail::NumericVec T, std::size_t N, int start, int end, int step>
struct VecView final
{
private:
    Vec<T, N> &_original_vec;
    std::array<size_t, Range<start, end, step>::Size()> _ref_index;

public:
    // 构造函数
    constexpr VecView(Vec<T, N> &original_vec, Range<start, end, step> range)
        requires(
            step != 0 &&
            []<int... Is>(std::integer_sequence<int, Is...>)
            {
                auto get_idx = [](int i)
                {
                    int val = start + i * step;
                    return (val < 0) ? (static_cast<int>(N) + val) : val;
                };
                return ((get_idx(Is) >= 0 && get_idx(Is) < static_cast<int>(N)) && ...);
            }(std::make_integer_sequence<int, Range<start, end, step>::Size()>{}))
        : _original_vec(original_vec)
    {
        size_t i = 0;
        for (auto val : range)
        {
            if (val < 0)
            {
                _ref_index[i++] = static_cast<size_t>(static_cast<int>(N) + val);
            }
            else
            {
                _ref_index[i++] = static_cast<size_t>(val);
            }
        }
    }

    // 赋值
    template <Detail::NumericVec U, size_t M>
        requires(Range<start, end, step>::Size() == M)
    constexpr VecView &operator=(const Vec<U, M> &other)
    {
        for (size_t i = 0; i < Range<start, end, step>::Size(); ++i)
        {
            _original_vec[_ref_index[i]] = static_cast<T>(other[i]);
        }
        return *this;
    }

    template <Detail::NumericVec U>
    constexpr VecView &operator=(const std::initializer_list<U> &list)
    {
        if (list.size() != Range<start, end, step>::Size())
        {
            throw std::runtime_error("Initializer list Size mismatch in VecView assignment");
        }
        size_t i = 0;
        for (const auto &val : list)
        {
            _original_vec[_ref_index[i++]] = static_cast<T>(val);
        }
        return *this;
    }

    template <Detail::NumericVec U>
    constexpr VecView &operator=(const std::array<U, Range<start, end, step>::Size()> &other)
    {
        for (size_t i = 0; i < Range<start, end, step>::Size(); ++i)
        {
            _original_vec[_ref_index[i]] = static_cast<T>(other[i]);
        }
        return *this;
    }

    template <Detail::NumericVec U>
    constexpr VecView &operator=(const std::array<U, Range<start, end, step>::Size()> &&other)
    {
        for (size_t i = 0; i < Range<start, end, step>::Size(); ++i)
        {
            _original_vec[_ref_index[i]] = static_cast<T>(other[i]);
        }
        return *this;
    }

    template <Detail::NumericVec U>
    constexpr VecView &operator=(const std::list<U> &other)
    {
        if (other.size() != Range<start, end, step>::Size())
        {
            throw std::runtime_error("List Size mismatch in VecView assignment");
        }
        size_t i = 0;
        for (const auto &val : other)
        {
            _original_vec[_ref_index[i++]] = static_cast<T>(val);
        }
        return *this;
    }

    template <Detail::NumericVec U>
    constexpr VecView &operator=(const std::vector<U> &other)
    {
        if (other.size() != Range<start, end, step>::Size())
        {
            throw std::runtime_error("Vector Size mismatch in VecView assignment");
        }
        for (size_t i = 0; i < Range<start, end, step>::Size(); ++i)
        {
            _original_vec[_ref_index[i]] = static_cast<T>(other[i]);
        }
        return *this;
    }

    template <Detail::NumericVec U>
    constexpr VecView &operator=(const std::span<U, Range<start, end, step>::Size()> &other)
    {
        for (size_t i = 0; i < Range<start, end, step>::Size(); ++i)
        {
            _original_vec[_ref_index[i]] = static_cast<T>(other[i]);
        }
        return *this;
    }

    // 隐式转换回 Vec
    template <Detail::NumericVec U>
    constexpr operator Vec<U, Range<start, end, step>::Size()>() const
    {
        Vec<U, Range<start, end, step>::Size()> result;

        size_t i = 0;
        for (size_t ref_index : _ref_index)
        {
            result[i] = static_cast<U>(_original_vec[ref_index]);
            i++;
        }

        return result;
    }

    // 获取视图大小
    constexpr size_t Size() const { return _ref_index.size(); }

    // 单元素访问
    constexpr T &operator[](size_t index)
    {
        assert(index < _ref_index.size());
        return _original_vec[_ref_index[index]];
    }

    constexpr const T &operator[](size_t index) const
    {
        assert(index < _ref_index.size());
        return _original_vec[_ref_index[index]];
    }
};

// 标量与向量的混合运算（非友元，避免模板重定义冲突）
template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator+(const Vec<T, N> &lhs, U rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(a, sv));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) + static_cast<ResultType>(rhs);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) + static_cast<ResultType>(rhs);
    }
    return result;
}

template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator+(U lhs, const Vec<T, N> &rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(sv, b));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs) + static_cast<ResultType>(rhs.m_data[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs) + static_cast<ResultType>(rhs.m_data[i]);
    }
    return result;
}

template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator-(const Vec<T, N> &lhs, U rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(a, sv));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) - static_cast<ResultType>(rhs);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) - static_cast<ResultType>(rhs);
    }
    return result;
}

template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator-(U lhs, const Vec<T, N> &rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(sv, b));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs) - static_cast<ResultType>(rhs.m_data[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs) - static_cast<ResultType>(rhs.m_data[i]);
    }
    return result;
}

template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator*(const Vec<T, N> &lhs, U rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Mul<ResultType>(a, sv));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) * static_cast<ResultType>(rhs);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) * static_cast<ResultType>(rhs);
    }
    return result;
}

template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator*(U lhs, const Vec<T, N> &rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Mul<ResultType>(sv, b));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs) * static_cast<ResultType>(rhs.m_data[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs) * static_cast<ResultType>(rhs.m_data[i]);
    }
    return result;
}

template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator/(const Vec<T, N> &lhs, U rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N> && !std::is_integral_v<ResultType>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Div<ResultType>(a, sv));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) / static_cast<ResultType>(rhs);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs.m_data[i]) / static_cast<ResultType>(rhs);
    }
    return result;
}

template <Detail::NumericVec T, std::size_t N, Detail::NumericVec U>
constexpr auto operator/(U lhs, const Vec<T, N> &rhs)
{
    using ResultType = std::common_type_t<T, U>;
    Vec<ResultType, N> result;
    if constexpr (Detail::VecUseSIMD<ResultType, N> && !std::is_integral_v<ResultType>)
    {
        constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
        auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
        std::size_t i = 0;
        for (; i + W <= N; i += W)
        {
            auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
            RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Div<ResultType>(sv, b));
        }
        for (; i < N; ++i)
        {
            result.m_data[i] = static_cast<ResultType>(lhs) / static_cast<ResultType>(rhs.m_data[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < N; ++i)
            result.m_data[i] = static_cast<ResultType>(lhs) / static_cast<ResultType>(rhs.m_data[i]);
    }
    return result;
}

// 常用向量类型
using Vec2i = Vec<int, 2>;
using Vec2f = Vec<float, 2>;
using Vec2d = Vec<double, 2>;
using Vec2l = Vec<long, 2>;
using Vec3i = Vec<int, 3>;
using Vec3f = Vec<float, 3>;
using Vec3d = Vec<double, 3>;
using Vec3l = Vec<long, 3>;
using Vec4i = Vec<int, 4>;
using Vec4f = Vec<float, 4>;
using Vec4d = Vec<double, 4>;
using Vec4l = Vec<long, 4>;

}