#pragma once

// =============================================================================
// Math 库单元测试
// -----------------------------------------------------------------------------
// 用法：在 main 中调用分组测试函数或总入口，例如：
//
//   #include "Core/Math/Test.hpp"
//   int main() {
//       int fails = RandEngine::Core::Math::Test::RunAllMathTests();
//       return fails == 0 ? 0 : 1;
//   }
//
// 也可单独调用某一分组：
//   RandEngine::Core::Math::Test::TestVecFunctions();
// =============================================================================

#include "Math.hpp"
#include <cstdio>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace RandEngine::Core::Math::Test
{

// =============================================================================
// 测试框架（断言 + 计数）
// =============================================================================

inline int g_checks = 0;
inline int g_failures = 0;

#define MATH_TEST_CHECK(...)                                                                   \
    do {                                                                                       \
        ++::RandEngine::Core::Math::Test::g_checks;                                            \
        if (!(__VA_ARGS__)) {                                                                  \
            ++::RandEngine::Core::Math::Test::g_failures;                                      \
            std::printf("    [FAIL] %s:%d\n", __FILE__, __LINE__);                             \
        }                                                                                      \
    } while (0)

// 容差比较
inline bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }
inline bool Near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

template <typename T, std::size_t N>
inline bool VecNear(const Vec<T, N> &a, const Vec<T, N> &b, T eps)
{
    for (std::size_t i = 0; i < N; ++i)
        if (!Near(static_cast<double>(a[i]), static_cast<double>(b[i]), static_cast<double>(eps))) return false;
    return true;
}

template <typename T, std::size_t R, std::size_t C>
inline bool MatNear(const Mat<T, R, C> &a, const Mat<T, R, C> &b, T eps)
{
    for (std::size_t i = 0; i < R * C; ++i)
        if (!Near(static_cast<double>(a[i]), static_cast<double>(b[i]), static_cast<double>(eps))) return false;
    return true;
}

// =============================================================================
// 1. 标量函数
// =============================================================================
inline void TestScalarFunctions()
{
    std::printf("[Test] 标量函数\n");
    MATH_TEST_CHECK(Clamp(5, 0, 10) == 5);
    MATH_TEST_CHECK(Clamp(-3, 0, 10) == 0);
    MATH_TEST_CHECK(Clamp(15, 0, 10) == 10);
    MATH_TEST_CHECK(Clamp(2.5, 0.0, 1.0) == 1.0);

    MATH_TEST_CHECK(Near(Lerp(0.0, 10.0, 0.5), 5.0));
    MATH_TEST_CHECK(Near(Lerp(0.0f, 10.0f, 0.25f), 2.5f));

    MATH_TEST_CHECK(Near(SmoothStep(0.0, 10.0, 5.0), 0.5));
    MATH_TEST_CHECK(Near(SmoothStep(0.0, 10.0, 0.0), 0.0));
    MATH_TEST_CHECK(Near(SmoothStep(0.0, 10.0, 10.0), 1.0));

    MATH_TEST_CHECK(Step(5, 3) == 0);
    MATH_TEST_CHECK(Step(5, 7) == 1);

    MATH_TEST_CHECK(Near(Remap(5.0, 0.0, 10.0, 0.0, 100.0), 50.0));
    MATH_TEST_CHECK(Near(Remap(0.0, 0.0, 10.0, 0.0, 100.0), 0.0));
    MATH_TEST_CHECK(Near(Remap(10.0, 0.0, 10.0, 0.0, 100.0), 100.0));

    MATH_TEST_CHECK(Near(DegToRad(180.0), 3.14159265358979));
    MATH_TEST_CHECK(Near(RadToDeg(3.14159265358979), 180.0));
    MATH_TEST_CHECK(Near(DegToRad(90.0f), 1.5707963267948966f));

    MATH_TEST_CHECK(ApproxEqual(1.0, 1.0 + 1e-9));
    MATH_TEST_CHECK(!ApproxEqual(1.0, 2.0));
    MATH_TEST_CHECK(ApproxEqual(1.0f, 1.0000001f));
}

// =============================================================================
// 2. Vec 构造 / 转换
// =============================================================================
inline void TestVecConstruction()
{
    std::printf("[Test] Vec 构造/转换\n");
    using V3 = Vec<float, 3>;
    using V4 = Vec<int, 4>;

    // 默认 = 全 0
    V3 a;
    MATH_TEST_CHECK(VecNear(a, V3{0, 0, 0}, 1e-6f));
    // 标量填充
    V3 b(2.0f);
    MATH_TEST_CHECK(VecNear(b, V3{2, 2, 2}, 1e-6f));
    // initializer_list
    V3 c{1, 2, 3};
    MATH_TEST_CHECK(VecNear(c, V3{1, 2, 3}, 1e-6f));
    // 变参模板
    V3 d(4, 5, 6);
    MATH_TEST_CHECK(VecNear(d, V3{4, 5, 6}, 1e-6f));
    // 复制 / 移动
    V3 e = c;
    MATH_TEST_CHECK(e == c);
    V3 f = std::move(c);
    MATH_TEST_CHECK(VecNear(f, V3{1, 2, 3}, 1e-6f));

    // 指针 / array / list / vector / span
    float arr[3] = {7, 8, 9};
    MATH_TEST_CHECK(VecNear(V3(arr), V3{7, 8, 9}, 1e-6f));
    MATH_TEST_CHECK(VecNear(V3(std::array<float, 3>{7, 8, 9}), V3{7, 8, 9}, 1e-6f));
    MATH_TEST_CHECK(VecNear(V3(std::list<float>{7, 8, 9}), V3{7, 8, 9}, 1e-6f));
    MATH_TEST_CHECK(VecNear(V3(std::vector<float>{7, 8, 9}), V3{7, 8, 9}, 1e-6f));
    MATH_TEST_CHECK(VecNear(V3(std::span<const float, 3>(arr)), V3{7, 8, 9}, 1e-6f));

    // 类型转换构造
    Vec<double, 3> g{1, 2, 3};
    V3 h(g);
    MATH_TEST_CHECK(VecNear(h, V3{1, 2, 3}, 1e-6f));

    // MakeZero
    MATH_TEST_CHECK(VecNear(V3::MakeZero(), V3{0, 0, 0}, 1e-6f));

    // 反向转换
    std::array<float, 3> arr2 = static_cast<std::array<float, 3>>(h);
    MATH_TEST_CHECK(arr2[0] == 1.0f && arr2[2] == 3.0f);
    std::vector<float> vec = static_cast<std::vector<float>>(h);
    MATH_TEST_CHECK(vec.size() == 3 && vec[1] == 2.0f);
    std::list<float> lst = static_cast<std::list<float>>(h);
    MATH_TEST_CHECK(lst.size() == 3);

    // 指针转换
    const float *p = static_cast<const float *>(h);
    MATH_TEST_CHECK(p && p[0] == 1.0f);

    // 类型别名
    V4 vi{1, 2, 3, 4};
    MATH_TEST_CHECK(vi == V4{1, 2, 3, 4});
    Vec3f v3f{1, 2, 3};
    Vec2d v2d{1, 2};
    Vec4l v4l{1, 2, 3, 4};
    MATH_TEST_CHECK(v3f.Size() == 3 && v2d.Size() == 2 && v4l.Size() == 4);
}

// =============================================================================
// 3. Vec 访问
// =============================================================================
inline void TestVecAccess()
{
    std::printf("[Test] Vec 访问\n");
    Vec4f v{10, 20, 30, 40};

    MATH_TEST_CHECK(v[0] == 10.0f);
    MATH_TEST_CHECK(v[3] == 40.0f);

    // 编译期索引
    MATH_TEST_CHECK(v[Detail::CompileTimeIndexCheckVec<4>(std::integral_constant<std::size_t, 2>{})] == 30.0f);

    MATH_TEST_CHECK(v.X() == 10.0f);
    MATH_TEST_CHECK(v.Y() == 20.0f);
    MATH_TEST_CHECK(v.Z() == 30.0f);
    MATH_TEST_CHECK(v.W() == 40.0f);

    // 可写引用
    v.X() = 11;
    MATH_TEST_CHECK(v.X() == 11.0f);
    v.X() = 10;

    // XYZ / RGB (const 版本返回 Vec)
    const Vec4f &cv = v;
    Vec<float, 3> xyz = cv.XYZ();
    MATH_TEST_CHECK(VecNear(xyz, Vec<float, 3>{10, 20, 30}, 1e-6f));
    Vec<float, 3> rgb = cv.RGB();
    MATH_TEST_CHECK(VecNear(rgb, Vec<float, 3>{10, 20, 30}, 1e-6f));

    // Range 索引 (const 返回 Vec)
    Vec<float, 2> part = cv[Range<0, 2, 1>()];
    MATH_TEST_CHECK(VecNear(part, Vec<float, 2>{10, 20}, 1e-6f));

    // 负索引 range：Range<-2,2,1> 生成 {-2,-1,0,1} → {v[2],v[3],v[0],v[1]}
    Vec<float, 4> neg = cv[Range<-2, 2, 1>()];
    MATH_TEST_CHECK(VecNear(neg, Vec<float, 4>{30, 40, 10, 20}, 1e-6f));

    // 查询方法
    MATH_TEST_CHECK(v.Size() == 4);
    MATH_TEST_CHECK(v.SizeInBytes() == 4 * sizeof(float));
}

// =============================================================================
// 4. Vec Set 方法
// =============================================================================
inline void TestVecSet()
{
    std::printf("[Test] Vec Set 方法\n");
    Vec4f v{1, 2, 3, 4};

    v.SetZero();
    MATH_TEST_CHECK(VecNear(v, Vec4f{0, 0, 0, 0}, 1e-6f));

    v.SetValue(7);
    MATH_TEST_CHECK(VecNear(v, Vec4f{7, 7, 7, 7}, 1e-6f));

    v.SetX(1);
    v.SetY(2);
    v.SetZ(3);
    v.SetW(4);
    MATH_TEST_CHECK(VecNear(v, Vec4f{1, 2, 3, 4}, 1e-6f));

    v.SetXYZ(5, 6, 7);
    MATH_TEST_CHECK(v[0] == 5 && v[1] == 6 && v[2] == 7);

    v.SetXYZ(Vec3f{8, 9, 10});
    MATH_TEST_CHECK(v[0] == 8 && v[1] == 9 && v[2] == 10);

    v.SetRGB(1, 2, 3);
    MATH_TEST_CHECK(v[0] == 1 && v[1] == 2 && v[2] == 3);
    v.SetRGB(Vec3f{4, 5, 6});
    MATH_TEST_CHECK(v[0] == 4 && v[1] == 5 && v[2] == 6);
}

// =============================================================================
// 5. Vec 算术运算
// =============================================================================
inline void TestVecArithmetic()
{
    std::printf("[Test] Vec 算术\n");
    Vec4f a{1, 2, 3, 4}, b{5, 6, 7, 8};

    MATH_TEST_CHECK(VecNear(a + b, Vec4f{6, 8, 10, 12}, 1e-3f));
    MATH_TEST_CHECK(VecNear(b - a, Vec4f{4, 4, 4, 4}, 1e-3f));
    MATH_TEST_CHECK(VecNear(a * b, Vec4f{5, 12, 21, 32}, 1e-3f));
    MATH_TEST_CHECK(VecNear(a / b, Vec4f{1.0f / 5, 2.0f / 6, 3.0f / 7, 4.0f / 8}, 1e-3f));
    MATH_TEST_CHECK(VecNear(-a, Vec4f{-1, -2, -3, -4}, 1e-6f));

    // 标量混合
    MATH_TEST_CHECK(VecNear(a + 2.0f, Vec4f{3, 4, 5, 6}, 1e-3f));
    MATH_TEST_CHECK(VecNear(2.0f + a, Vec4f{3, 4, 5, 6}, 1e-3f));
    MATH_TEST_CHECK(VecNear(a - 2.0f, Vec4f{-1, 0, 1, 2}, 1e-3f));
    MATH_TEST_CHECK(VecNear(2.0f - a, Vec4f{1, 0, -1, -2}, 1e-3f));
    MATH_TEST_CHECK(VecNear(a * 2.0f, Vec4f{2, 4, 6, 8}, 1e-3f));
    MATH_TEST_CHECK(VecNear(2.0f * a, Vec4f{2, 4, 6, 8}, 1e-3f));
    MATH_TEST_CHECK(VecNear(a / 2.0f, Vec4f{0.5f, 1, 1.5f, 2}, 1e-3f));
    MATH_TEST_CHECK(VecNear(2.0f / a, Vec4f{2.0f, 1.0f, 2.0f / 3, 0.5f}, 1e-3f));

    // 复合赋值
    Vec4f c = a;
    c += b;
    MATH_TEST_CHECK(VecNear(c, Vec4f{6, 8, 10, 12}, 1e-3f));
    c = a; c += 2.0f;
    MATH_TEST_CHECK(VecNear(c, Vec4f{3, 4, 5, 6}, 1e-3f));
    c = a; c -= b;
    MATH_TEST_CHECK(VecNear(c, Vec4f{-4, -4, -4, -4}, 1e-3f));
    c = a; c -= 2.0f;
    MATH_TEST_CHECK(VecNear(c, Vec4f{-1, 0, 1, 2}, 1e-3f));
    c = a; c *= b;
    MATH_TEST_CHECK(VecNear(c, Vec4f{5, 12, 21, 32}, 1e-3f));
    c = a; c *= 2.0f;
    MATH_TEST_CHECK(VecNear(c, Vec4f{2, 4, 6, 8}, 1e-3f));
    c = a; c /= b;
    MATH_TEST_CHECK(VecNear(c, Vec4f{1.0f / 5, 2.0f / 6, 3.0f / 7, 4.0f / 8}, 1e-3f));
    c = a; c /= 2.0f;
    MATH_TEST_CHECK(VecNear(c, Vec4f{0.5f, 1, 1.5f, 2}, 1e-3f));

    // 叉积复合赋值 (N==3)
    Vec3f x{1, 0, 0}, y{0, 1, 0};
    x ^= y;
    MATH_TEST_CHECK(VecNear(x, Vec3f{0, 0, 1}, 1e-6f));

    // 比较 (字典序)
    MATH_TEST_CHECK(a == a);
    MATH_TEST_CHECK(a != b);
    MATH_TEST_CHECK(a < b);

    // 赋值
    Vec4f d;
    d = 9.0f;
    MATH_TEST_CHECK(VecNear(d, Vec4f{9, 9, 9, 9}, 1e-6f));

    // double / int
    Vec4d ad{1, 2, 3, 4}, bd{5, 6, 7, 8};
    MATH_TEST_CHECK(VecNear(ad + bd, Vec4d{6, 8, 10, 12}, 1e-9));
    Vec4i ai{1, 2, 3, 4}, bi{5, 6, 7, 8};
    MATH_TEST_CHECK(ai + bi == Vec4i{6, 8, 10, 12});
    MATH_TEST_CHECK(ai * bi == Vec4i{5, 12, 21, 32});
}

// =============================================================================
// 6. VecView
// =============================================================================
inline void TestVecView()
{
    std::printf("[Test] VecView\n");
    Vec4f v{1, 2, 3, 4};

    // 非 const 视图可写
    auto view = v[Range<0, 3, 1>()];
    view = Vec<float, 3>{10, 20, 30};
    MATH_TEST_CHECK(VecNear(v, Vec4f{10, 20, 30, 4}, 1e-6f));

    // initializer_list 赋值
    v[Range<0, 2, 1>()] = {7, 8};
    MATH_TEST_CHECK(VecNear(v, Vec4f{7, 8, 30, 4}, 1e-6f));

    // array / vector / list 赋值
    v[Range<0, 2, 1>()] = std::array<float, 2>{1, 2};
    MATH_TEST_CHECK(VecNear(v, Vec4f{1, 2, 30, 4}, 1e-6f));
    v[Range<0, 2, 1>()] = std::vector<float>{3, 4};
    MATH_TEST_CHECK(VecNear(v, Vec4f{3, 4, 30, 4}, 1e-6f));
    v[Range<0, 2, 1>()] = std::list<float>{5, 6};
    MATH_TEST_CHECK(VecNear(v, Vec4f{5, 6, 30, 4}, 1e-6f));

    // 隐式转换回 Vec
    Vec<float, 3> back = v[Range<0, 3, 1>()];
    MATH_TEST_CHECK(VecNear(back, Vec<float, 3>{5, 6, 30}, 1e-6f));

    // 单元素访问
    auto view2 = v[Range<0, 4, 1>()];
    MATH_TEST_CHECK(view2[2] == 30.0f);
    view2[3] = 40;
    MATH_TEST_CHECK(v[3] == 40.0f);
}

// =============================================================================
// 7. Range
// =============================================================================
inline void TestRange()
{
    std::printf("[Test] Range\n");
    MATH_TEST_CHECK(Range<0, 5, 1>::Size() == 5);
    MATH_TEST_CHECK(Range<0, 10, 2>::Size() == 5);
    MATH_TEST_CHECK(Range<5, 0, -1>::Size() == 5);
    MATH_TEST_CHECK(Range<0, 0, 1>::Size() == 0);

    // 迭代
    {
        std::vector<int> got;
        for (int i : Range<0, 4, 1>()) got.push_back(i);
        MATH_TEST_CHECK((got == std::vector<int>{0, 1, 2, 3}));
    }
    {
        std::vector<int> got;
        for (int i : Range<3, 0, -1>()) got.push_back(i);
        MATH_TEST_CHECK((got == std::vector<int>{3, 2, 1}));
    }
    {
        std::vector<int> got;
        for (int i : Range<1, 6, 2>()) got.push_back(i);
        MATH_TEST_CHECK((got == std::vector<int>{1, 3, 5}));
    }

    // 索引访问
    Range<0, 5, 1> r;
    MATH_TEST_CHECK(r[0] == 0 && r[2] == 2 && r[4] == 4);
    MATH_TEST_CHECK(Range<1, 6, 2>()[2] == 5);

    // Values()
    auto vals = Range<0, 4, 1>::Values();
    MATH_TEST_CHECK(vals[0] == 0 && vals[3] == 3);

    // 转换
    std::vector<int> v = Range<0, 3, 1>();
    MATH_TEST_CHECK((v == std::vector<int>{0, 1, 2}));
    std::array<int, 3> a = Range<0, 3, 1>();
    MATH_TEST_CHECK(a[2] == 2);
    std::list<int> l = Range<0, 3, 1>();
    MATH_TEST_CHECK(l.size() == 3);

    // 越界抛异常
    bool threw = false;
    try { (void)r[100]; }
    catch (const std::out_of_range &) { threw = true; }
    MATH_TEST_CHECK(threw);
}

// =============================================================================
// 8. Vec 计算函数
// =============================================================================
inline void TestVecFunctions()
{
    std::printf("[Test] Vec 计算函数\n");
    using V3 = Vec3f;

    // 长度 / 模长
    V3 a{3, 4, 0};
    MATH_TEST_CHECK(Near(LengthSquared(a), 25.0f, 1e-3f));
    MATH_TEST_CHECK(Near(Length(a), 5.0f, 1e-3f));
    MATH_TEST_CHECK(VecNear(Normalize(a), V3{0.6f, 0.8f, 0.0f}, 1e-3f));
    MATH_TEST_CHECK(VecNear(Normalize(V3{0, 0, 0}), V3{0, 0, 0}, 1e-6f));

    // 点积
    V3 b{4, 5, 6};
    MATH_TEST_CHECK(Near(Dot(a, b), 32.0f, 1e-3f));
    // 多参数点积：sum_i a[i]*b[i]*c[i]
    V3 c{2, 2, 2};
    MATH_TEST_CHECK(Near(Dot(a, b, c), 64.0f, 1e-3f));

    // 叉积
    MATH_TEST_CHECK(VecNear(Cross(V3{1, 0, 0}, V3{0, 1, 0}), V3{0, 0, 1}, 1e-6f));
    MATH_TEST_CHECK(VecNear(V3{1, 0, 0} ^ V3{0, 1, 0}, V3{0, 0, 1}, 1e-6f));
    MATH_TEST_CHECK(Near(Cross(Vec2f{1, 2}, Vec2f{3, 4}), -2.0f, 1e-6f));

    // Hadamard (2 / 3 参数)
    MATH_TEST_CHECK(VecNear(Hadamard(V3{1, 2, 3}, V3{4, 5, 6}), V3{4, 10, 18}, 1e-3f));
    MATH_TEST_CHECK(VecNear(Hadamard(V3{1, 2, 3}, V3{4, 5, 6}, V3{1, 1, 1}), V3{4, 10, 18}, 1e-3f));

    // Cat 拼接
    MATH_TEST_CHECK(VecNear(Cat(Vec2f{1, 2}, Vec2f{3, 4}), Vec4f{1, 2, 3, 4}, 1e-6f));

    // 距离
    MATH_TEST_CHECK(Near(DistanceSquared(V3{0, 0, 0}, V3{3, 4, 0}), 25.0f, 1e-3f));
    MATH_TEST_CHECK(Near(Distance(V3{0, 0, 0}, V3{3, 4, 0}), 5.0f, 1e-3f));

    // Lerp (Vec)
    MATH_TEST_CHECK(VecNear(Lerp(V3{0, 0, 0}, V3{10, 10, 10}, 0.5f), V3{5, 5, 5}, 1e-3f));

    // Project
    MATH_TEST_CHECK(VecNear(Project(V3{3, 4, 0}, V3{1, 0, 0}), V3{3, 0, 0}, 1e-3f));

    // Reflect：a={1,-1,0} 关于 n={0,1,0} → {1,1,0}
    MATH_TEST_CHECK(VecNear(Reflect(V3{1, -1, 0}, V3{0, 1, 0}), V3{1, 1, 0}, 1e-3f));

    // Radian / Degree
    MATH_TEST_CHECK(Near(Radian(V3{1, 0, 0}, V3{0, 1, 0}), 1.57079632679));
    MATH_TEST_CHECK(Near(Degree(V3{1, 0, 0}, V3{0, 1, 0}), 90.0));
    MATH_TEST_CHECK(Near(Radian(V3{1, 0, 0}, V3{1, 0, 0}), 0.0));

    // Abs
    MATH_TEST_CHECK(VecNear(Abs(V3{-1, 2, -3}), V3{1, 2, 3}, 1e-6f));
    MATH_TEST_CHECK(VecNear(Abs(Vec4i{-1, -2, 3, 4}), Vec4i{1, 2, 3, 4}, 0));

    // Min / Max
    MATH_TEST_CHECK(VecNear(Min(V3{1, 5, 3}, V3{4, 2, 6}), V3{1, 2, 3}, 1e-6f));
    MATH_TEST_CHECK(VecNear(Max(V3{1, 5, 3}, V3{4, 2, 6}), V3{4, 5, 6}, 1e-6f));

    // Clamp (Vec 范围 / 标量范围)
    MATH_TEST_CHECK(VecNear(Clamp(V3{-5, 3, 9}, V3{0, 0, 0}, V3{4, 4, 4}), V3{0, 3, 4}, 1e-6f));
    MATH_TEST_CHECK(VecNear(Clamp(V3{-5, 3, 9}, 0.0f, 4.0f), V3{0, 3, 4}, 1e-6f));

    // ClampMagnitude
    MATH_TEST_CHECK(VecNear(ClampMagnitude(V3{3, 4, 0}, 3.0f), V3{1.8f, 2.4f, 0}, 1e-3f));
    MATH_TEST_CHECK(VecNear(ClampMagnitude(V3{1, 0, 0}, 5.0f), V3{1, 0, 0}, 1e-6f));

    // Sum / Mean
    MATH_TEST_CHECK(Near(Sum(V3{1, 2, 3}), 6.0f, 1e-3f));
    MATH_TEST_CHECK(Near(Mean(V3{1, 2, 3}), 2.0f, 1e-3f));

    // Max/Min 分量
    MATH_TEST_CHECK(Near(MaxComponent(V3{3, 9, 4}), 9.0f, 1e-6f));
    MATH_TEST_CHECK(Near(MinComponent(V3{3, 9, 4}), 3.0f, 1e-6f));
    MATH_TEST_CHECK(MaxIndex(V3{3, 9, 4}) == 1);
    MATH_TEST_CHECK(MinIndex(V3{3, 9, 4}) == 0);

    // Nlerp / Slerp
    MATH_TEST_CHECK(VecNear(Nlerp(V3{1, 0, 0}, V3{0, 1, 0}, 0.5f), Normalize(V3{1, 1, 0}), 1e-3f));
    {
        V3 sl = Slerp(V3{1, 0, 0}, V3{0, 1, 0}, 0.5f);
        MATH_TEST_CHECK(VecNear(sl, V3{0.70710678f, 0.70710678f, 0}, 1e-3f));
    }
    // 近共线退化为 Lerp
    MATH_TEST_CHECK(VecNear(Slerp(V3{1, 0, 0}, V3{1, 0.0001f, 0}, 0.5f), Lerp(V3{1, 0, 0}, V3{1, 0.0001f, 0}, 0.5f), 1e-3f));

    // InverseLerp
    MATH_TEST_CHECK(VecNear(InverseLerp(V3{0, 0, 0}, V3{10, 10, 10}, V3{2, 5, 7}), V3{0.2f, 0.5f, 0.7f}, 1e-3f));

    // SmoothStep (Vec)
    MATH_TEST_CHECK(VecNear(SmoothStep(V3{0, 0, 0}, V3{10, 10, 10}, 5.0f), V3{0.5f, 0.5f, 0.5f}, 1e-3f));

    // OuterProduct → Mat
    {
        auto O = OuterProduct(V3{1, 2, 3}, V3{4, 5, 6});
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                MATH_TEST_CHECK(Near(O[r, c], (r + 1) * (c + 4), 1e-3f));
    }

    // TripleProduct
    MATH_TEST_CHECK(Near(TripleProduct(V3{1, 0, 0}, V3{0, 1, 0}, V3{0, 0, 1}), 1.0f, 1e-3f));

    // ProjectOnPlane
    MATH_TEST_CHECK(VecNear(ProjectOnPlane(V3{1, 1, 1}, V3{0, 0, 1}), V3{1, 1, 0}, 1e-3f));

    // Refract：垂直入射 eta=1 折射不变
    MATH_TEST_CHECK(VecNear(Refract(V3{0, 0, -1}, V3{0, 0, 1}, 1.0f), V3{0, 0, -1}, 1e-3f));
    // 全反射返回零向量 (I⊥n, eta=2 → k=1-4<0)
    MATH_TEST_CHECK(VecNear(Refract(V3{1, 0, 0}, V3{0, 0, 1}, 2.0f), V3{0, 0, 0}, 1e-3f));

    // Faceforward：Dot(Nref,I)<0 返回 n，否则返回 -n
    MATH_TEST_CHECK(VecNear(Faceforward(V3{0, 0, 1}, V3{0, 0, -1}, V3{0, 0, -1}), V3{0, 0, -1}, 1e-6f));
    MATH_TEST_CHECK(VecNear(Faceforward(V3{0, 0, 1}, V3{0, 0, 1}, V3{0, 0, -1}), V3{0, 0, 1}, 1e-6f));

    // IsParallel / IsOrthogonal / Midpoint
    MATH_TEST_CHECK(IsParallel(V3{1, 0, 0}, V3{2, 0, 0}));
    MATH_TEST_CHECK(!IsParallel(V3{1, 0, 0}, V3{0, 1, 0}));
    MATH_TEST_CHECK(IsOrthogonal(V3{1, 0, 0}, V3{0, 1, 0}));
    MATH_TEST_CHECK(!IsOrthogonal(V3{1, 1, 0}, V3{1, 1, 0}));
    MATH_TEST_CHECK(VecNear(Midpoint(V3{0, 0, 0}, V3{2, 4, 6}), V3{1, 2, 3}, 1e-3f));

    // double 路径
    Vec4d ad{1, 2, 3, 4}, bd{5, 6, 7, 8};
    MATH_TEST_CHECK(Near(Dot(ad, bd), 70.0, 1e-9));
    MATH_TEST_CHECK(Near(Length(Vec4d{3, 4, 0, 0}), 5.0, 1e-9));
}

// =============================================================================
// 9. Mat 构造 / 静态工厂
// =============================================================================
inline void TestMatConstruction()
{
    std::printf("[Test] Mat 构造/静态工厂\n");
    using M2 = Mat2f;
    using M3 = Mat3f;
    using M4 = Mat4f;

    // 默认 = 全 0
    M2 a;
    MATH_TEST_CHECK(MatNear(a, M2{0, 0, 0, 0}, 1e-6f));
    // 标量填充
    M2 b(2.0f);
    MATH_TEST_CHECK(MatNear(b, M2{2, 2, 2, 2}, 1e-6f));
    // initializer_list 平铺 (row-major)
    M2 c{1, 2, 3, 4};
    MATH_TEST_CHECK(c[0, 0] == 1 && c[0, 1] == 2 && c[1, 0] == 3 && c[1, 1] == 4);
    // 行构造
    M2 d{{1, 2}, {3, 4}};
    MATH_TEST_CHECK(d[0, 0] == 1 && d[0, 1] == 2 && d[1, 0] == 3 && d[1, 1] == 4);
    // 变参
    M2 e(1, 2, 3, 4);
    MATH_TEST_CHECK(MatNear(e, c, 1e-6f));

    // array / list / vector / span 构造
    MATH_TEST_CHECK(MatNear(M2(std::array<float, 4>{1, 2, 3, 4}), c, 1e-6f));
    MATH_TEST_CHECK(MatNear(M2(std::vector<float>{1, 2, 3, 4}), c, 1e-6f));
    MATH_TEST_CHECK(MatNear(M2(std::list<float>{1, 2, 3, 4}), c, 1e-6f));

    // 类型转换构造
    Mat2d cd{1, 2, 3, 4};
    M2 cf(cd);
    MATH_TEST_CHECK(MatNear(cf, c, 1e-6f));

    // 转换运算符
    std::array<float, 4> arr = static_cast<std::array<float, 4>>(c);
    MATH_TEST_CHECK(arr[0] == 1 && arr[3] == 4);
    std::vector<float> vec = static_cast<std::vector<float>>(c);
    MATH_TEST_CHECK(vec.size() == 4);

    // MakeIdentity
    M3 I = M3::MakeIdentity();
    MATH_TEST_CHECK(I[0, 0] == 1 && I[0, 1] == 0 && I[2, 2] == 1);

    // MakeScale
    M3 S = M3::MakeScale(Vec3f{2, 3, 4});
    MATH_TEST_CHECK(S[0, 0] == 2 && S[1, 1] == 3 && S[2, 2] == 4 && S[0, 1] == 0);

    // MakeTranslation (4x4)
    M4 T = M4::MakeTranslation(Vec3f{1, 2, 3});
    MATH_TEST_CHECK(T[3, 0] == 1 && T[3, 1] == 2 && T[3, 2] == 3 && T[0, 0] == 1);

    // MakeRotation (2x2)：旋转 90°
    M2 R = M2::MakeRotation(1.57079632679f);
    MATH_TEST_CHECK(Near(R[0, 0], 0.0f, 1e-3f) && Near(R[0, 1], -1.0f, 1e-3f));
    MATH_TEST_CHECK(Near(R[1, 0], 1.0f, 1e-3f) && Near(R[1, 1], 0.0f, 1e-3f));

    // MakeRotationX/Y/Z (3x3)：正交性 M*M^T = I
    M3 RX = M3::MakeRotationX(0.5f);
    M3 RY = M3::MakeRotationY(0.5f);
    M3 RZ = M3::MakeRotationZ(0.5f);
    MATH_TEST_CHECK(MatNear(RX * Transpose(RX), M3::MakeIdentity(), 1e-3f));
    MATH_TEST_CHECK(MatNear(RY * Transpose(RY), M3::MakeIdentity(), 1e-3f));
    MATH_TEST_CHECK(MatNear(RZ * Transpose(RZ), M3::MakeIdentity(), 1e-3f));

    // MakeView (3D LookAt) / MakeLookAt（同一 eye 应一致）
    M4 V = M4::MakeView(Vec3f{0, 0, 5}, Vec3f{0, 0, -1}, Vec3f{0, 1, 0});
    MATH_TEST_CHECK(V.Size() == 16);
    M4 L = M4::MakeLookAt(Vec3f{0, 0, 5}, Vec3f{0, 0, 0}, Vec3f{0, 1, 0});
    MATH_TEST_CHECK(MatNear(L, V, 1e-3f));

    // MakeView (2D)
    M4 V2 = M4::MakeView(Vec2f{0, 0}, 0.0f, 1.0f);
    MATH_TEST_CHECK(Near(V2[0, 0], 1.0f, 1e-6f) && Near(V2[1, 1], 1.0f, 1e-6f));

    // MakeProjection (fov 版)
    M4 P = M4::MakeProjection(1.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    MATH_TEST_CHECK(P[1, 1] > 0 && P[3, 2] == -1.0f);
    // MakeProjection (正交版)
    M4 PO = M4::MakeProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    MATH_TEST_CHECK(PO[0, 0] == 1.0f && PO[1, 1] == 1.0f);

    // 类型别名
    Mat4d m4d{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    MATH_TEST_CHECK(m4d.Size() == 16);
    Mat3i m3i{1, 2, 3, 4, 5, 6, 7, 8, 9};
    MATH_TEST_CHECK(m3i.RowSize() == 3 && m3i.ColSize() == 3);
}

// =============================================================================
// 10. Mat 访问 / Set 方法
// =============================================================================
inline void TestMatAccess()
{
    std::printf("[Test] Mat 访问/Set 方法\n");
    Mat3f m{1, 2, 3, 4, 5, 6, 7, 8, 9};

    MATH_TEST_CHECK(m[0] == 1 && m[8] == 9);
    MATH_TEST_CHECK(m[1, 1] == 5 && m[0, 2] == 3);
    MATH_TEST_CHECK(m[Detail::CompileTimeIndexCheckMat<9>(std::integral_constant<std::size_t, 4>{})] == 5);

    // GetRow / GetCol
    Mat<float, 1, 3> row = m.GetRow(1);
    MATH_TEST_CHECK(row[0] == 4 && row[1] == 5 && row[2] == 6);
    Mat<float, 3, 1> col = m.GetCol(2);
    MATH_TEST_CHECK(col[0] == 3 && col[1] == 6 && col[2] == 9);

    // SetRow / SetCol / SetIdentity / SetValue
    m.SetRow(0, Vec3f{9, 8, 7});
    MATH_TEST_CHECK(m[0, 0] == 9 && m[0, 2] == 7);
    m.SetCol(0, Vec3f{1, 2, 3});
    MATH_TEST_CHECK(m[0, 0] == 1 && m[1, 0] == 2 && m[2, 0] == 3);
    m.SetIdentity();
    MATH_TEST_CHECK(m[0, 0] == 1 && m[1, 1] == 1 && m[0, 1] == 0);
    m.SetValue(5);
    MATH_TEST_CHECK(MatNear(m, Mat3f{5, 5, 5, 5, 5, 5, 5, 5, 5}, 1e-6f));

    // 赋值
    m = 3.0f;
    MATH_TEST_CHECK(MatNear(m, Mat3f{3, 3, 3, 3, 3, 3, 3, 3, 3}, 1e-6f));

    // 异常：initializer_list size 不匹配
    bool threw = false;
    try { Mat3f bad{1, 2, 3}; }
    catch (const std::runtime_error &) { threw = true; }
    MATH_TEST_CHECK(threw);
}

// =============================================================================
// 11. Mat 算术运算
// =============================================================================
inline void TestMatArithmetic()
{
    std::printf("[Test] Mat 算术\n");
    using M4 = Mat4f;
    M4 A{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    M4 B{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    // Mat ± Mat
    M4 Sum = A + B;
    MATH_TEST_CHECK(MatNear(Sum, M4{17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17}, 1e-3f));
    M4 Diff = A - B;
    MATH_TEST_CHECK(Diff[0, 0] == -15 && Diff[0, 1] == -13);

    // 标量混合
    MATH_TEST_CHECK(MatNear(A + 1.0f, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]+1; return R; }(), 1e-3f));
    MATH_TEST_CHECK(MatNear(1.0f + A, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]+1; return R; }(), 1e-3f));
    MATH_TEST_CHECK(MatNear(A - 1.0f, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]-1; return R; }(), 1e-3f));
    MATH_TEST_CHECK(MatNear(1.0f - A, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=1-A[i]; return R; }(), 1e-3f));
    MATH_TEST_CHECK(MatNear(A * 2.0f, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]*2; return R; }(), 1e-3f));
    MATH_TEST_CHECK(MatNear(2.0f * A, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]*2; return R; }(), 1e-3f));

    // 一元负
    MATH_TEST_CHECK(MatNear(-A, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=-A[i]; return R; }(), 1e-6f));

    // 矩阵乘 A*B（与标量参考对比）
    M4 P = A * B;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
        {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += A[r, k] * B[k, c];
            MATH_TEST_CHECK(Near(P[r, c], s, 1e-2f));
        }

    // Mat * Vec, Vec * Mat
    Vec4f v{1, 2, 3, 4};
    Vec4f mv = A * v;
    for (int r = 0; r < 4; ++r)
    {
        float s = 0;
        for (int k = 0; k < 4; ++k) s += A[r, k] * v[k];
        MATH_TEST_CHECK(Near(mv[r], s, 1e-2f));
    }
    Vec4f vm = v * A;
    for (int c = 0; c < 4; ++c)
    {
        float s = 0;
        for (int k = 0; k < 4; ++k) s += v[k] * A[k, c];
        MATH_TEST_CHECK(Near(vm[c], s, 1e-2f));
    }

    // 复合赋值
    M4 C = A;
    C += B;
    MATH_TEST_CHECK(MatNear(C, Sum, 1e-3f));
    C = A; C += 1.0f;
    MATH_TEST_CHECK(MatNear(C, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]+1; return R; }(), 1e-3f));
    C = A; C -= B;
    MATH_TEST_CHECK(MatNear(C, Diff, 1e-3f));
    C = A; C -= 1.0f;
    MATH_TEST_CHECK(MatNear(C, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]-1; return R; }(), 1e-3f));
    C = A; C *= B;
    MATH_TEST_CHECK(MatNear(C, P, 1e-2f));
    C = A; C *= 2.0f;
    MATH_TEST_CHECK(MatNear(C, [&]{ M4 R; for (int i=0;i<16;++i) R[i]=A[i]*2; return R; }(), 1e-3f));

    // v *= m
    Vec4f vv{1, 0, 0, 0};
    vv *= A;
    MATH_TEST_CHECK(VecNear(vv, Vec4f{1, 2, 3, 4}, 1e-3f));

    // double 矩阵乘 (W=2 SIMD)
    Mat4d D{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    Mat4d E{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    MATH_TEST_CHECK(MatNear(D * E, D, 1e-9));
}

// =============================================================================
// 12. MatView
// =============================================================================
inline void TestMatView()
{
    std::printf("[Test] MatView\n");
    Mat4f m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    // 非 const 视图赋值（左上 2x2）
    m[Range<0, 2, 1>(), Range<0, 2, 1>()] = Mat2f{9, 8, 7, 6};
    MATH_TEST_CHECK(m[0, 0] == 9 && m[0, 1] == 8 && m[1, 0] == 7 && m[1, 1] == 6);
    MATH_TEST_CHECK(m[0, 2] == 3 && m[2, 0] == 9);

    // initializer_list 赋值
    m[Range<2, 4, 1>(), Range<2, 4, 1>()] = {1, 2, 3, 4};
    MATH_TEST_CHECK(m[2, 2] == 1 && m[2, 3] == 2 && m[3, 2] == 3 && m[3, 3] == 4);

    // array 赋值
    m[Range<0, 2, 1>(), Range<2, 4, 1>()] = std::array<float, 4>{5, 6, 7, 8};
    MATH_TEST_CHECK(m[0, 2] == 5 && m[0, 3] == 6 && m[1, 2] == 7 && m[1, 3] == 8);

    // 隐式转换回 Mat
    Mat2f sub = m[Range<0, 2, 1>(), Range<0, 2, 1>()];
    MATH_TEST_CHECK(sub[0, 0] == 9 && sub[1, 1] == 6);
}

// =============================================================================
// 13. Mat 计算函数
// =============================================================================
inline void TestMatFunctions()
{
    std::printf("[Test] Mat 计算函数\n");
    using M2 = Mat2f;
    using M3 = Mat3f;

    // Transpose
    M2 T{1, 2, 3, 4};
    M2 Tt = Transpose(T);
    MATH_TEST_CHECK(Tt[0, 0] == 1 && Tt[0, 1] == 3 && Tt[1, 0] == 2 && Tt[1, 1] == 4);

    // Det
    MATH_TEST_CHECK(Near(Det(M2{1, 2, 3, 4}), -2.0f, 1e-3f));
    MATH_TEST_CHECK(Near(Det(M3{2, 0, 0, 0, 3, 0, 0, 0, 4}), 24.0f, 1e-3f));

    // Cofactor
    M2 Cof{1, 2, 3, 4};
    MATH_TEST_CHECK(Near(Cofactor(Cof, 0, 0), 4.0f, 1e-3f));
    MATH_TEST_CHECK(Near(Cofactor(Cof, 0, 1), -3.0f, 1e-3f));

    // Adjoint (伴随)
    M2 Adj{1, 2, 3, 4};
    M2 AdjRef{4, -2, -3, 1};
    MATH_TEST_CHECK(MatNear(Adjoint(Adj), AdjRef, 1e-3f));

    // Inverse：A * A⁻¹ ≈ I
    M2 Inv{4, 7, 2, 6};
    M2 InvI = Inverse(Inv);
    MATH_TEST_CHECK(MatNear(Inv * InvI, M2::MakeIdentity(), 1e-3f));
    M3 Inv3{1, 2, 3, 0, 1, 4, 5, 6, 0};
    MATH_TEST_CHECK(MatNear(Inv3 * Inverse(Inv3), M3::MakeIdentity(), 1e-3f));

    // Trace
    MATH_TEST_CHECK(Near(Trace(M3{1, 0, 0, 0, 2, 0, 0, 0, 3}), 6.0f, 1e-6f));

    // Rank / IsFullRank
    MATH_TEST_CHECK(Rank(M2{1, 0, 0, 1}) == 2);
    MATH_TEST_CHECK(Rank(M2{1, 2, 2, 4}) == 1);
    MATH_TEST_CHECK(IsFullRank(M2{1, 0, 0, 1}));
    MATH_TEST_CHECK(!IsFullRank(M2{1, 2, 2, 4}));

    // MakeDiagonal / Diagonal
    M3 Diag = MakeDiagonal(Vec3f{1, 2, 3});
    MATH_TEST_CHECK(Diag[0, 0] == 1 && Diag[1, 1] == 2 && Diag[2, 2] == 3);
    MATH_TEST_CHECK(VecNear(Diagonal(Diag), Vec3f{1, 2, 3}, 1e-6f));

    // 判定
    MATH_TEST_CHECK(IsIdentity(M3::MakeIdentity()));
    MATH_TEST_CHECK(!IsIdentity(M3{1, 0, 0, 0, 1, 0, 0, 0, 2}));
    MATH_TEST_CHECK(IsDiagonal(Diag));
    MATH_TEST_CHECK(!IsDiagonal(M3{1, 1, 0, 0, 2, 0, 0, 0, 3}));
    MATH_TEST_CHECK(IsSymmetric(M3{1, 2, 0, 2, 3, 0, 0, 0, 4}));
    MATH_TEST_CHECK(IsSkewSymmetric(M3{0, 1, 0, -1, 0, 0, 0, 0, 0}));
    MATH_TEST_CHECK(IsOrthogonal(M2::MakeRotation(0.7f)));
    MATH_TEST_CHECK(IsSingular(M2{1, 2, 2, 4}));

    // FrobeniusNorm
    MATH_TEST_CHECK(Near(FrobeniusNorm(M2{3, 4, 0, 0}), 5.0f, 1e-3f));

    // SolveLinearSystem：[[2,1],[1,3]]x = [5,10] → x = [1,3]
    {
        M2 A{2, 1, 1, 3};
        Vec2f b{5, 10};
        Vec2f x = SolveLinearSystem(A, b);
        MATH_TEST_CHECK(VecNear(x, Vec2f{1, 3}, 1e-3f));
    }

    // MatrixPower：[[2,0],[0,3]]^2 = [[4,0],[0,9]]
    {
        M2 A{2, 0, 0, 3};
        M2 A2 = MatrixPower(A, 2);
        MATH_TEST_CHECK(MatNear(A2, M2{4, 0, 0, 9}, 1e-3f));
        MATH_TEST_CHECK(MatNear(MatrixPower(A, 0), M2::MakeIdentity(), 1e-6f));
        MATH_TEST_CHECK(MatNear(MatrixPower(A, 1), A, 1e-6f));
    }

    // PseudoInverse（满秩方阵等于普通逆）
    {
        M2 A{4, 7, 2, 6};
        MATH_TEST_CHECK(MatNear(PseudoInverse(A), Inverse(A), 1e-3f));
    }

    // KroneckerProduct / Kronecker
    {
        M2 A{1, 2, 3, 4};
        M2 B{0, 5, 6, 7};
        auto K = KroneckerProduct(A, B);
        MATH_TEST_CHECK(K.Size() == 16);
        MATH_TEST_CHECK(K[0, 0] == 0 && K[0, 1] == 5);
        MATH_TEST_CHECK(K[2, 0] == 0 && K[2, 1] == 15); // 3 * B
        // Kron 与 KronProduct 一致
        MATH_TEST_CHECK(MatNear(Kronecker(A, B), K, 1e-6f));
    }

    // Mat Hadamard
    {
        M3 A{1, 2, 3, 4, 5, 6, 7, 8, 9};
        M3 B{1, 1, 1, 2, 2, 2, 3, 3, 3};
        M3 H = Hadamard(A, B);
        MATH_TEST_CHECK(H[0, 0] == 1 && H[1, 1] == 10 && H[2, 2] == 27);
        // double 路径
        Mat3d C{1, 2, 3, 4, 5, 6, 7, 8, 9};
        Mat3d D{1, 1, 1, 1, 1, 1, 1, 1, 1};
        MATH_TEST_CHECK(MatNear(Hadamard(C, D), C, 1e-9));
    }

    // 异常：奇异矩阵求逆
    bool threw = false;
    try { (void)Inverse(M2{1, 2, 2, 4}); }
    catch (const std::runtime_error &) { threw = true; }
    MATH_TEST_CHECK(threw);
}

// =============================================================================
// 14. 输出运算符
// =============================================================================
inline void TestOutputOperators()
{
    std::printf("[Test] 输出运算符\n");
    std::ostringstream oss;
    Vec3f v{1, 2, 3};
    oss << v;
    MATH_TEST_CHECK(oss.str() == "[1, 2, 3]");

    std::ostringstream oss2;
    Mat2f m{1, 2, 3, 4};
    oss2 << m;
    MATH_TEST_CHECK(!oss2.str().empty());

    // 视图输出
    std::ostringstream oss3;
    Vec4f v4{1, 2, 3, 4};
    oss3 << v4[Range<0, 3, 1>()];
    MATH_TEST_CHECK(oss3.str() == "[1, 2, 3]");
}

// =============================================================================
// 总入口
// =============================================================================
inline int RunAllMathTests()
{
    std::printf("========== Math 库单元测试 ==========\n");
    g_checks = 0;
    g_failures = 0;

    TestScalarFunctions();
    TestVecConstruction();
    TestVecAccess();
    TestVecSet();
    TestVecArithmetic();
    TestVecView();
    TestRange();
    TestVecFunctions();
    TestMatConstruction();
    TestMatAccess();
    TestMatArithmetic();
    TestMatView();
    TestMatFunctions();
    TestOutputOperators();

    std::printf("=====================================\n");
    std::printf("总计: %d 项断言, %d 项失败\n", g_checks, g_failures);
    std::printf(g_failures == 0 ? "全部通过 ✓\n" : "存在失败 ✗\n");
    return g_failures;
}

} // namespace RandEngine::Core::Math::Test
