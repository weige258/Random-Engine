#pragma once

#include <tuple>
#include <concepts>
#include <array>
#include <cmath>
#include <cassert>
#include "Vec.hpp"
#include "Range.hpp"

namespace RandomEngine::Core::Math
{

    namespace Detail
    {
        template <typename T>
        concept NumericMat = std::is_arithmetic_v<T>;

        template <NumericMat T>
        constexpr bool IsNearZero(T val)
        {
            if constexpr (std::is_floating_point_v<T>)
                return std::abs(val) < static_cast<T>(1e-6);
            else
                return val == 0;
        }

        // 编译期索引检查 (与 Vec 的 CompileTimeIndexCheckVec 对称)
        template <std::size_t Limit>
        struct CompileTimeIndexCheckMat
        {
            std::size_t value;

            template <std::size_t I>
            consteval CompileTimeIndexCheckMat(std::integral_constant<std::size_t, I>) : value(I)
            {
                static_assert(I < Limit, "Mat index out of bounds!");
            }
        };

        // 多维initlist构造行数据
        template <Detail::NumericMat T, size_t Col>
        struct RowData
        {
            std::array<T, Col> row_values;

            template <typename... Args>
            constexpr RowData(Args &&...args)
                : row_values{static_cast<T>(args)...} {}
        };

        template <typename T>
        struct NullEmptyPtr
        {
            const T *ptr;
            consteval NullEmptyPtr(std::nullptr_t)
            {
                static_assert(sizeof(T) == 0, "Mat cannot be initialized with nullptr!");
            }
            consteval NullEmptyPtr(const T *p) : ptr(p) {}

            constexpr T operator[](size_t i) const { return ptr[i]; }
        };

        template <typename T, size_t Row, size_t Col>
        inline constexpr bool MatUseSIMD = RandomEngine::Platform::SIMD::SupportsSIMD<T> && (RandomEngine::Platform::SIMD::SIMDWidth<T> > 1) && (Row * Col >= RandomEngine::Platform::SIMD::SIMDWidth<T>);

        // 内部辅助：按索引收集元素构成 SIMD 向量（等价于临时 simd::gather）
        template <typename T>
        inline auto Gather(const T* base, const std::size_t* idx)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
            alignas(RandomEngine::Platform::SIMD::SIMDAlignment) T tmp[W];
            for (std::size_t k = 0; k < W; ++k)
                tmp[k] = base[idx[k]];
            return RandomEngine::Platform::SIMD::LoadU<T>(tmp);
        }
    }

    // 矩阵视图类型声明
    template <Detail::NumericMat T, size_t Row, size_t Col, typename RowRange, typename ColRange>
    struct MatView;

    // 矩阵类型
    template <Detail::NumericMat T, size_t Row, size_t Col>
    struct Mat final
    {
    private:
        std::array<T, Row * Col> m_data;

    public:
        using mat_type_alias = T;

        // 标量混合运算符的友元声明
        template <Detail::NumericMat U, size_t R, size_t C, Detail::NumericMat V>
        friend constexpr auto operator+(const Mat<U, R, C> &lhs, V rhs);
        template <Detail::NumericMat U, size_t R, size_t C, Detail::NumericMat V>
        friend constexpr auto operator+(V lhs, const Mat<U, R, C> &rhs);
        template <Detail::NumericMat U, size_t R, size_t C, Detail::NumericMat V>
        friend constexpr auto operator-(const Mat<U, R, C> &lhs, V rhs);
        template <Detail::NumericMat U, size_t R, size_t C, Detail::NumericMat V>
        friend constexpr auto operator-(V lhs, const Mat<U, R, C> &rhs);
        template <Detail::NumericMat U, size_t R, size_t C, Detail::NumericMat V>
        friend constexpr auto operator*(const Mat<U, R, C> &lhs, V rhs);
        template <Detail::NumericMat U, size_t R, size_t C, Detail::NumericMat V>
        friend constexpr auto operator*(V lhs, const Mat<U, R, C> &rhs);

    public:
        // 构造
        constexpr Mat() { m_data.fill(0); }

        constexpr Mat(T num) { m_data.fill(num); }

        constexpr Mat(const Mat &other) = default;

        constexpr Mat(Mat &&other) noexcept = default;

        template <Detail::NumericMat U>
        constexpr Mat(const std::initializer_list<U> &list)
        {
            if (list.size() != Row * Col)
            {
                throw std::runtime_error("Size mismatch");
            }
            size_t i = 0;
            for (const auto &val : list)
            {
                m_data[i++] = static_cast<T>(val);
            }
        }

        constexpr Mat(const std::initializer_list<Detail::RowData<T, Col>> &list)
        {
            if (list.size() != Row)
            {
                throw std::runtime_error("Row count mismatch");
            }
            size_t r = 0;
            for (const auto &row : list)
            {
                for (size_t c = 0; c < Col; ++c)
                {
                    m_data[r * Col + c] = row.row_values[c];
                }
                r++;
            }
        }

        template <typename... Args>
        constexpr Mat(const Args &...args)
            requires(sizeof...(args) == Row * Col && (std::convertible_to<Args, T> && ...))
        {
            m_data = std::array<T, Row * Col>{static_cast<T>(args)...};
        };

        template <typename U, size_t OtherRow, size_t OtherCol>
        constexpr Mat(const Mat<U, OtherRow, OtherCol> &other)
        {
            static_assert(OtherRow == Row && OtherCol == Col, "Matrix dimension mismatch: Row and Col must be equal for conversion.");
            for (size_t i = 0; i < Row * Col; ++i)
            {
                m_data[i] = static_cast<T>(other[i]);
            }
        }

        template <Detail::NumericMat U>
        constexpr Mat(Mat<U, Row, Col> &&other)
        {
            for (size_t i = 0; i < Row * Col; ++i)
            {
                m_data[i] = static_cast<T>(other[i]);
            }
        }

        constexpr Mat(const Detail::NullEmptyPtr<T> arr)
        {
            for (size_t i = 0; i < Row * Col; ++i)
            {
                m_data[i] = arr[i];
            }
        }

        constexpr Mat(const std::array<T, Row * Col> &arr) : m_data(arr) {}

        template <Detail::NumericMat U>
        constexpr Mat(const std::list<U> &list)
            requires std::convertible_to<U, T>
        {
            if (list.size() != Row * Col)
            {
                throw std::runtime_error("Size mismatch");
            }

            std::copy(list.begin(), list.end(), m_data.begin());
        }

        template <Detail::NumericMat U>
        constexpr Mat(const std::vector<U> &vec)
            requires std::convertible_to<U, T>
        {
            if (vec.size() != Row * Col)
            {
                throw std::runtime_error("Size mismatch");
            }

            std::copy(vec.begin(), vec.end(), m_data.begin());
        }

        constexpr Mat(const std::span<const T, Row * Col> &span)
        {
            std::copy(span.begin(), span.end(), m_data.begin());
        }

        static constexpr Mat<T, Row, Col> MakeIdentity()
        {
            static_assert(Row == Col, "Identity matrix must be square.");
            Mat<T, Row, Col> res;
            for (size_t i = 0; i < Row; ++i)
                res[i, i] = 1;
            return res;
        }

        // 2D 旋转矩阵 (仅 2x2)
        static constexpr Mat<T, 2, 2> MakeRotation(T radians)
            requires(Row == 2 && Col == 2)
        {
            Mat<T, 2, 2> r;
            T c = std::cos(radians);
            T s = std::sin(radians);
            r[0, 0] = c;
            r[0, 1] = -s;
            r[1, 0] = s;
            r[1, 1] = c;
            return r;
        }

        // 3D 旋转矩阵 (仅 3x3)
        static constexpr Mat<T, 3, 3> MakeRotationX(T radians)
            requires(Row == 3 && Col == 3)
        {
            Mat<T, 3, 3> r = Mat<T, 3, 3>::MakeIdentity();
            T c = std::cos(radians);
            T s = std::sin(radians);
            r[1, 1] = c;
            r[1, 2] = -s;
            r[2, 1] = s;
            r[2, 2] = c;
            return r;
        }

        static constexpr Mat<T, 3, 3> MakeRotationY(T radians)
            requires(Row == 3 && Col == 3)
        {
            Mat<T, 3, 3> r = Mat<T, 3, 3>::MakeIdentity();
            T c = std::cos(radians);
            T s = std::sin(radians);
            r[0, 0] = c;
            r[0, 2] = s;
            r[2, 0] = -s;
            r[2, 2] = c;
            return r;
        }

        static constexpr Mat<T, 3, 3> MakeRotationZ(T radians)
            requires(Row == 3 && Col == 3)
        {
            Mat<T, 3, 3> r = Mat<T, 3, 3>::MakeIdentity();
            T c = std::cos(radians);
            T s = std::sin(radians);
            r[0, 0] = c;
            r[0, 1] = -s;
            r[1, 0] = s;
            r[1, 1] = c;
            return r;
        }

        // 缩放矩阵 (仅方阵)
        template <Detail::NumericVec U>
            requires(Row == Col)
        static constexpr Mat<T, Row, Col> MakeScale(const Vec<U, Row> &scale)
        {
            Mat<T, Row, Col> result = Mat<T, Row, Col>::MakeIdentity();
            for (size_t i = 0; i < Row; ++i)
                result[i, i] = static_cast<T>(scale[i]);
            return result;
        }

        // 平移矩阵 (仅 4x4)
        template <Detail::NumericVec U>
            requires(Row == 4 && Col == 4)
        static constexpr Mat<T, 4, 4> MakeTranslation(const Vec<U, 3> &translation)
        {
            Mat<T, 4, 4> result = Mat<T, 4, 4>::MakeIdentity();
            result[3, 0] = static_cast<T>(translation[0]);
            result[3, 1] = static_cast<T>(translation[1]);
            result[3, 2] = static_cast<T>(translation[2]);
            return result;
        }

        // 3D 视图矩阵 (仅 4x4)
        template <Detail::NumericVec U>
            requires(Row == 4 && Col == 4)
        static constexpr Mat<T, 4, 4> MakeView(const Vec<U, 3> &camera_pos, const Vec<U, 3> &camera_direction, const Vec<U, 3> &camera_up)
        {
            Vec<T, 3> direction = Vec<T, 3>(camera_direction);
            Vec<T, 3> front = Normalize(direction);
            Vec<T, 3> right = Normalize(direction ^ Vec<T, 3>(camera_up));
            Vec<T, 3> up = Normalize(right ^ direction);

            Mat<T, 4, 4> rotation_matrix = Mat<T, 4, 4>::MakeIdentity();
            Mat<T, 4, 4> translation_matrix = Mat<T, 4, 4>::MakeIdentity();

            rotation_matrix[0, 0] = right[0];
            rotation_matrix[0, 1] = right[1];
            rotation_matrix[0, 2] = right[2];
            rotation_matrix[1, 0] = up[0];
            rotation_matrix[1, 1] = up[1];
            rotation_matrix[1, 2] = up[2];
            rotation_matrix[2, 0] = front[0];
            rotation_matrix[2, 1] = front[1];
            rotation_matrix[2, 2] = front[2];

            translation_matrix[3, 0] = -static_cast<T>(camera_pos[0]);
            translation_matrix[3, 1] = -static_cast<T>(camera_pos[1]);
            translation_matrix[3, 2] = -static_cast<T>(camera_pos[2]);

            return rotation_matrix * translation_matrix;
        }

        // 2D 视图矩阵 (仅 4x4)
        static constexpr Mat<T, 4, 4> MakeView(const Vec<T, 2> &pos, T rotation_radians, T zoom)
            requires(Row == 4 && Col == 4)
        {
            Mat<T, 4, 4> mat{};
            T cos_r = std::cos(rotation_radians);
            T sin_r = std::sin(rotation_radians);

            mat[0, 0] = cos_r * zoom;
            mat[0, 1] = sin_r * zoom;
            mat[0, 3] = -(pos[0] * cos_r + pos[1] * sin_r) * zoom;

            mat[1, 0] = -sin_r * zoom;
            mat[1, 1] = cos_r * zoom;
            mat[1, 3] = (pos[0] * sin_r - pos[1] * cos_r) * zoom;

            mat[2, 2] = 1;
            mat[3, 3] = 1;
            return mat;
        }

        // 视图矩阵 (LookAt, 仅 4x4, 复用 MakeView)
        template <Detail::NumericVec U>
            requires(Row == 4 && Col == 4)
        static constexpr Mat<T, 4, 4> MakeLookAt(const Vec<U, 3> &eye, const Vec<U, 3> &center, const Vec<U, 3> &up)
        {
            return MakeView(eye, center - eye, up);
        }

        // 透视投影矩阵 (仅 4x4)
        static constexpr Mat<T, 4, 4> MakeProjection(T fov_radians, T aspect, T near, T far)
            requires(Row == 4 && Col == 4)
        {
            Mat<T, 4, 4> mat{};
            T tanHalfFov = std::tan(fov_radians / static_cast<T>(2));

            mat[0, 0] = static_cast<T>(1) / (aspect * tanHalfFov);
            mat[1, 1] = static_cast<T>(1) / tanHalfFov;
            mat[2, 2] = -(far + near) / (far - near);
            mat[2, 3] = -(static_cast<T>(2) * far * near) / (far - near);
            mat[3, 2] = static_cast<T>(-1);
            return mat;
        }

        // 正交投影矩阵 (仅 4x4)
        static constexpr Mat<T, 4, 4> MakeProjection(T left, T right, T bottom, T top, T near, T far)
            requires(Row == 4 && Col == 4)
        {
            Mat<T, 4, 4> mat{};

            mat[0, 0] = static_cast<T>(2) / (right - left);
            mat[1, 1] = static_cast<T>(2) / (top - bottom);
            mat[2, 2] = -static_cast<T>(2) / (far - near);

            mat[0, 3] = -(right + left) / (right - left);
            mat[1, 3] = -(top + bottom) / (top - bottom);
            mat[2, 3] = -(far + near) / (far - near);
            mat[3, 3] = static_cast<T>(1);
            return mat;
        }

        // 简单正交投影矩阵 (仅 4x4)
        static constexpr Mat<T, 4, 4> MakeProjection(T width, T height)
            requires(Row == 4 && Col == 4)
        {
            T half_w = width / static_cast<T>(2);
            T half_h = height / static_cast<T>(2);
            return MakeProjection(-half_w, half_w, half_h, -half_h, static_cast<T>(-1), static_cast<T>(1));
        }

        // ===================== 实例 Set 方法 =====================

        // 就地填充标量
        constexpr Mat &SetValue(T value)
        {
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                auto v = RandomEngine::Platform::SIMD::Set1<T>(value);
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], v);
                }
                for (; i < Row * Col; ++i)
                {
                    m_data[i] = value;
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                    m_data[i] = value;
            }
            return *this;
        }

        // 就地设为单元矩阵 (仅方阵)
        constexpr Mat &SetIdentity()
        {
            static_assert(Row == Col, "Identity matrix must be square.");
            m_data.fill(static_cast<T>(0));
            for (size_t i = 0; i < Row; ++i)
                m_data[i * Col + i] = static_cast<T>(1);
            return *this;
        }

        // 就地设置第 row 行
        template <Detail::NumericVec U>
        constexpr Mat &SetRow(size_t row, const Vec<U, Col> &values)
        {
            assert(row < Row);
            for (size_t c = 0; c < Col; ++c)
                m_data[row * Col + c] = static_cast<T>(values[c]);
            return *this;
        }

        // 就地设置第 col 列
        template <Detail::NumericVec U>
        constexpr Mat &SetCol(size_t col, const Vec<U, Row> &values)
        {
            assert(col < Col);
            for (size_t r = 0; r < Row; ++r)
                m_data[r * Col + col] = static_cast<T>(values[r]);
            return *this;
        }

        // 析构
        ~Mat() = default;

        // 数据转化
        template <Detail::NumericMat U>
        constexpr operator std::array<U, Row *Col>() const
        {
            std::array<U, Row * Col> result{};
            for (size_t i = 0; i < Row * Col; ++i)
            {
                result[i] = static_cast<U>(m_data[i]);
            }
            return result;
        }

        template <Detail::NumericMat U>
        constexpr operator std::list<U>() const
        {
            return std::list<U>(m_data.begin(), m_data.end());
        }

        template <Detail::NumericMat U>
        constexpr operator std::vector<U>() const
        {
            return std::vector<U>(m_data.begin(), m_data.end());
        }

        constexpr operator std::span<T, Row *Col>()
        {
            return std::span<T, Row * Col>(m_data.data(), Row * Col);
        }

        constexpr operator std::span<const T, Row *Col>() const
        {
            return std::span<const T, Row * Col>(m_data.data(), Row * Col);
        }

        // 指针转换
        explicit operator T *() { return m_data.data(); }
        explicit operator const T *() const { return m_data.data(); }

        // 访问
        constexpr T &operator[](size_t index)
        {
            assert(index < Row * Col);
            return m_data[index];
        }

        constexpr const T &operator[](size_t index) const
        {
            assert(index < Row * Col);
            return m_data[index];
        }

        constexpr T &operator[](size_t row, size_t col)
        {
            assert(row < Row && col < Col);
            return m_data[row * Col + col];
        }

        constexpr const T &operator[](size_t row, size_t col) const
        {
            assert(row < Row && col < Col);
            return m_data[row * Col + col];
        }

        constexpr T &operator[](Detail::CompileTimeIndexCheckMat<Row * Col> index)
        {
            return m_data[index.value];
        }

        constexpr const T &operator[](Detail::CompileTimeIndexCheckMat<Row * Col> index) const
        {
            return m_data[index.value];
        }

        template <int RStart, int REnd, int RStep,
                  int CStart, int CEnd, int CStep>
        constexpr auto operator[](Range<RStart, REnd, RStep> rr,
                                  Range<CStart, CEnd, CStep> rc)
        {
            return MatView<T, Row, Col, decltype(rr), decltype(rc)>(*this, rr, rc);
        }

        constexpr Mat<T, 1, Col> GetRow(size_t row) const
        {
            Mat<T, 1, Col> result;

            size_t j = 0;
            for (size_t i = row * Col; i < row * Col + Col; ++i)
            {
                result[j] = (m_data[i]);
                j++;
            }

            return result;
        }

        constexpr Mat<T, Row, 1> GetCol(size_t col) const
        {
            Mat<T, Row, 1> result;

            for (size_t i = 0; i < Row; ++i)
            {
                result[i] = (m_data[i * Col + col]);
            }

            return result;
        }

        // 赋值
        constexpr Mat &operator=(const Mat &other) = default;

        constexpr Mat &operator=(Mat &&other) noexcept = default;

        constexpr Mat &operator=(const T &value)
        {
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                auto v = RandomEngine::Platform::SIMD::Set1<T>(value);
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], v);
                }
                for (; i < Row * Col; ++i)
                {
                    m_data[i] = value;
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    m_data[i] = value;
                }
            }
            return *this;
        }

        // 运算

        template <Detail::NumericMat U>
        constexpr friend auto operator+(const Mat<T, Row, Col> &lhs, const Mat<U, Row, Col> &rhs)
        {
            using ResultType = std::common_type_t<T, U>;
            Mat<ResultType, Row, Col> result;
            if constexpr (Detail::CanUseSIMD<T, U> && Detail::MatUseSIMD<ResultType, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                    auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(a, b));
                }
                for (; i < Row * Col; ++i)
                {
                    result[i] = static_cast<ResultType>(lhs[i]) + static_cast<ResultType>(rhs[i]);
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    result[i] = static_cast<ResultType>(lhs[i]) + static_cast<ResultType>(rhs[i]);
                }
            }
            return result;
        }

        template <Detail::NumericMat U>
        constexpr friend auto operator-(const Mat<T, Row, Col> &lhs, const Mat<U, Row, Col> &rhs)
        {
            using ResultType = std::common_type_t<T, U>;
            Mat<ResultType, Row, Col> result;
            if constexpr (Detail::CanUseSIMD<T, U> && Detail::MatUseSIMD<ResultType, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                    auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(a, b));
                }
                for (; i < Row * Col; ++i)
                {
                    result[i] = static_cast<ResultType>(lhs[i]) - static_cast<ResultType>(rhs[i]);
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    result[i] = static_cast<ResultType>(lhs[i]) - static_cast<ResultType>(rhs[i]);
                }
            }
            return result;
        }

        template <Detail::NumericMat U, size_t OtherCol>
        constexpr friend auto operator*(const Mat<T, Row, Col> &lhs, const Mat<U, Col, OtherCol> &rhs)
        {
            using ResultType = std::common_type_t<T, U>;
            Mat<ResultType, Row, OtherCol> result;
            if constexpr (Detail::CanUseSIMD<T, U> &&
                          (Col % RandomEngine::Platform::SIMD::SIMDWidth<ResultType> == 0) &&
                          Detail::MatUseSIMD<ResultType, Row, OtherCol>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
                constexpr std::size_t Blocks = Col / W;
                for (std::size_t r = 0; r < Row; ++r)
                {
                    for (std::size_t c = 0; c < OtherCol; ++c)
                    {
                        auto sum = RandomEngine::Platform::SIMD::Zero<ResultType>();
                        for (std::size_t b = 0; b < Blocks; ++b)
                        {
                            auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[r * Col + b * W]);
                            std::size_t idx[W];
                            for (std::size_t k = 0; k < W; ++k)
                            {
                                idx[k] = (b * W + k) * OtherCol + c;
                            }
                            auto rb = Detail::Gather<ResultType>(&rhs.m_data[0], idx);
                            sum = RandomEngine::Platform::SIMD::FMAdd<ResultType>(a, rb, sum);
                        }
                        result[r * OtherCol + c] = RandomEngine::Platform::SIMD::HAdd<ResultType>(sum);
                    }
                }
            }
            else
            {
                for (size_t r = 0; r < Row; ++r)
                {
                    for (size_t c = 0; c < OtherCol; ++c)
                    {
                        ResultType sum = 0;
                        for (size_t k = 0; k < Col; ++k)
                        {
                            sum += static_cast<ResultType>(lhs[r * Col + k]) * static_cast<ResultType>(rhs[k * OtherCol + c]);
                        }
                        result[r * OtherCol + c] = sum;
                    }
                }
            }
            return result;
        }

        template <Detail::NumericVec U>
        constexpr friend auto operator*(const Mat<T, Row, Col> &lhs, const Vec<U, Col> &rhs)
        {
            using ResultType = std::common_type_t<T, U>;
            Vec<ResultType, Row> result;
            if constexpr (Detail::CanUseSIMD<T, U> &&
                          (Col % RandomEngine::Platform::SIMD::SIMDWidth<ResultType> == 0) &&
                          Detail::VecUseSIMD<ResultType, Row>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
                constexpr std::size_t Blocks = Col / W;
                for (std::size_t r = 0; r < Row; ++r)
                {
                    auto sum = RandomEngine::Platform::SIMD::Zero<ResultType>();
                    for (std::size_t b = 0; b < Blocks; ++b)
                    {
                        auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[r * Col + b * W]);
                        auto vb = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs[b * W]);
                        sum = RandomEngine::Platform::SIMD::FMAdd<ResultType>(a, vb, sum);
                    }
                    result[r] = RandomEngine::Platform::SIMD::HAdd<ResultType>(sum);
                }
            }
            else
            {
                for (size_t r = 0; r < Row; ++r)
                {
                    ResultType sum = 0;
                    for (size_t c = 0; c < Col; ++c)
                    {
                        sum += static_cast<ResultType>(lhs[r * Col + c]) * static_cast<ResultType>(rhs[c]);
                    }
                    result[r] = sum;
                }
            }
            return result;
        }

        template <Detail::NumericVec U>
        constexpr friend auto operator*(const Vec<T, Row> &lhs, const Mat<U, Row, Col> &rhs)
        {
            using ResultType = std::common_type_t<T, U>;
            Vec<ResultType, Col> result;
            if constexpr (Detail::CanUseSIMD<T, U> &&
                          (Row % RandomEngine::Platform::SIMD::SIMDWidth<ResultType> == 0) &&
                          Detail::VecUseSIMD<ResultType, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
                constexpr std::size_t Blocks = Row / W;
                for (std::size_t c = 0; c < Col; ++c)
                {
                    auto sum = RandomEngine::Platform::SIMD::Zero<ResultType>();
                    for (std::size_t b = 0; b < Blocks; ++b)
                    {
                        auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs[b * W]);
                        std::size_t idx[W];
                        for (std::size_t k = 0; k < W; ++k)
                        {
                            idx[k] = (b * W + k) * Col + c;
                        }
                        auto rb = Detail::Gather<ResultType>(&rhs.m_data[0], idx);
                        sum = RandomEngine::Platform::SIMD::FMAdd<ResultType>(a, rb, sum);
                    }
                    result[c] = RandomEngine::Platform::SIMD::HAdd<ResultType>(sum);
                }
            }
            else
            {
                for (size_t c = 0; c < Col; ++c)
                {
                    ResultType sum = 0;
                    for (size_t r = 0; r < Row; ++r)
                    {
                        sum += static_cast<ResultType>(lhs[r]) * static_cast<ResultType>(rhs[r * Col + c]);
                    }
                    result[c] = sum;
                }
            }
            return result;
        }

        constexpr Mat<T, Row, Col> operator-() const
        {
            Mat<T, Row, Col> result;
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                auto z = RandomEngine::Platform::SIMD::Zero<T>();
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<T>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<T>(z, a));
                }
                for (; i < Row * Col; ++i)
                {
                    result[i] = -m_data[i];
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    result[i] = -m_data[i];
                }
            }
            return result;
        }

        // 复合赋值运算符

        constexpr Mat<T, Row, Col> &operator+=(const Mat<T, Row, Col> &other)
        {
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                    auto b = RandomEngine::Platform::SIMD::LoadU<T>(&other.m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Add<T>(a, b));
                }
                for (; i < Row * Col; ++i)
                {
                    m_data[i] += other.m_data[i];
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    m_data[i] += other.m_data[i];
                }
            }
            return *this;
        }

        constexpr Mat<T, Row, Col> &operator+=(const T &value)
        {
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                auto sv = RandomEngine::Platform::SIMD::Set1<T>(value);
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Add<T>(a, sv));
                }
                for (; i < Row * Col; ++i)
                {
                    m_data[i] += value;
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    m_data[i] += value;
                }
            }
            return *this;
        }

        constexpr Mat<T, Row, Col> &operator-=(const Mat<T, Row, Col> &other)
        {
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                    auto b = RandomEngine::Platform::SIMD::LoadU<T>(&other.m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Sub<T>(a, b));
                }
                for (; i < Row * Col; ++i)
                {
                    m_data[i] -= other.m_data[i];
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    m_data[i] -= other.m_data[i];
                }
            }
            return *this;
        }

        constexpr Mat<T, Row, Col> &operator-=(const T &value)
        {
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                auto sv = RandomEngine::Platform::SIMD::Set1<T>(value);
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Sub<T>(a, sv));
                }
                for (; i < Row * Col; ++i)
                {
                    m_data[i] -= value;
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    m_data[i] -= value;
                }
            }
            return *this;
        }

        template <Detail::NumericMat U>
        constexpr auto &operator*=(const Mat<U, Col, Col> &rhs)
        {
            static_assert(Row == Col, "operator*= is only supported for square matrices to maintain dimensions.");

            using R = std::common_type_t<T, U>;
            Mat<R, Row, Col> temp;
            if constexpr (Detail::CanUseSIMD<T, U> &&
                          (Col % RandomEngine::Platform::SIMD::SIMDWidth<R> == 0) &&
                          Detail::MatUseSIMD<R, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<R>;
                constexpr std::size_t Blocks = Col / W;
                for (std::size_t r = 0; r < Row; ++r)
                {
                    for (std::size_t c = 0; c < Col; ++c)
                    {
                        auto sum = RandomEngine::Platform::SIMD::Zero<R>();
                        for (std::size_t b = 0; b < Blocks; ++b)
                        {
                            auto a = RandomEngine::Platform::SIMD::LoadU<R>(&m_data[r * Col + b * W]);
                            std::size_t idx[W];
                            for (std::size_t k = 0; k < W; ++k)
                            {
                                idx[k] = (b * W + k) * Col + c;
                            }
                            auto rb = Detail::Gather<R>(&rhs.m_data[0], idx);
                            sum = RandomEngine::Platform::SIMD::FMAdd<R>(a, rb, sum);
                        }
                        temp[r * Col + c] = RandomEngine::Platform::SIMD::HAdd<R>(sum);
                    }
                }
            }
            else
            {
                for (size_t r = 0; r < Row; ++r)
                {
                    for (size_t c = 0; c < Col; ++c)
                    {
                        R sum = 0;
                        for (size_t k = 0; k < Col; ++k)
                        {
                            sum += static_cast<R>((*this)[r, k]) * static_cast<R>(rhs[k, c]);
                        }
                        temp[r, c] = sum;
                    }
                }
            }
            *this = Mat<T, Row, Col>(temp);
            return *this;
        }

        template <Detail::NumericMat U>
        constexpr Mat &operator*=(const U &scalar)
        {
            if constexpr (Detail::MatUseSIMD<T, Row, Col>)
            {
                constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<T>;
                auto sv = RandomEngine::Platform::SIMD::Set1<T>(static_cast<T>(scalar));
                std::size_t i = 0;
                for (; i + W <= Row * Col; i += W)
                {
                    auto a = RandomEngine::Platform::SIMD::LoadU<T>(&m_data[i]);
                    RandomEngine::Platform::SIMD::StoreU<T>(&m_data[i], RandomEngine::Platform::SIMD::Mul<T>(a, sv));
                }
                for (; i < Row * Col; ++i)
                {
                    m_data[i] = static_cast<T>(m_data[i] * scalar);
                }
            }
            else
            {
                for (size_t i = 0; i < Row * Col; ++i)
                {
                    m_data[i] = static_cast<T>(m_data[i] * scalar);
                }
            }
            return *this;
        }

        // 迭代器支持
        auto begin() noexcept { return m_data.begin(); }
        auto end() noexcept { return m_data.end(); }
        auto begin() const noexcept { return m_data.begin(); }
        auto end() const noexcept { return m_data.end(); }

        // 比较操作符 (C++20)
        auto operator<=>(const Mat &other) const = default;

        // 查询方法
        static constexpr size_t Size() { return Row * Col; };

        static constexpr size_t RowSize() { return Row; };

        static constexpr size_t ColSize() { return Col; };

        static constexpr std::tuple<size_t, size_t> Shape() { return std::make_tuple(Row, Col); };

        static const type_info &Type() noexcept { return typeid(Mat<T, Row, Col>); }

        static const type_info &ValueType() noexcept { return typeid(T); }
    };

    // 向量与矩阵复合乘法 v *= m（与 Mat*Vec / Vec*Mat 的 operator* 归位在同一头文件）
    template <Detail::NumericMat T, Detail::NumericMat U, size_t N>
    constexpr auto &operator*=(Vec<U, N> &lhs, const Mat<T, N, N> &rhs)
    {
        using R = std::common_type_t<T, U>;
        Vec<R, N> temp;
        for (size_t c = 0; c < N; ++c)
        {
            R sum = 0;
            for (size_t r = 0; r < N; ++r)
            {
                sum += static_cast<R>(lhs[r]) * static_cast<R>(rhs[r, c]);
            }
            temp[c] = sum;
        }
        lhs = Vec<U, N>(temp);
        return lhs;
    }

    template <typename T, typename U>
    inline constexpr bool CanConvertSIMD = !std::is_same_v<T, U> &&
        RandomEngine::Platform::SIMD::SupportsSIMD<T> &&
        RandomEngine::Platform::SIMD::SupportsSIMD<U> &&
        (std::is_same_v<T, float> || std::is_same_v<T, double>) &&
        (std::is_same_v<U, float> || std::is_same_v<U, double>);

    // 标量与矩阵的混合运算（非友元，避免模板重定义冲突）
    template <Detail::NumericMat T, size_t Row, size_t Col, Detail::NumericMat U>
    constexpr auto operator+(const Mat<T, Row, Col> &lhs, U rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Mat<ResultType, Row, Col> result;
        if constexpr (Detail::MatUseSIMD<ResultType, Row, Col> && std::is_same_v<T, ResultType>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
            std::size_t i = 0;
            for (; i + W <= Row * Col; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(a, sv));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) + static_cast<ResultType>(rhs);
        }
        else if constexpr (Detail::CanConvertSIMD<T, ResultType> && Detail::MatUseSIMD<T, Row, Col>)
        {
            constexpr std::size_t Step = RandomEngine::Platform::SIMD::ConvertStride<T, ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
            std::size_t i = 0;
            for (; i + Step <= Row * Col; i += Step)
            {
                auto a = RandomEngine::Platform::SIMD::LoadConvert<T, ResultType>(&lhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreConvert<T, ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(a, sv));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) + static_cast<ResultType>(rhs);
        }
        else
        {
            for (size_t i = 0; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) + static_cast<ResultType>(rhs);
        }
        return result;
    }

    template <Detail::NumericMat T, size_t Row, size_t Col, Detail::NumericMat U>
    constexpr auto operator+(U lhs, const Mat<T, Row, Col> &rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Mat<ResultType, Row, Col> result;
        if constexpr (Detail::MatUseSIMD<ResultType, Row, Col> && std::is_same_v<T, ResultType>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
            std::size_t i = 0;
            for (; i + W <= Row * Col; i += W)
            {
                auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(sv, b));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) + static_cast<ResultType>(rhs[i]);
        }
        else if constexpr (Detail::CanConvertSIMD<T, ResultType> && Detail::MatUseSIMD<T, Row, Col>)
        {
            constexpr std::size_t Step = RandomEngine::Platform::SIMD::ConvertStride<T, ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
            std::size_t i = 0;
            for (; i + Step <= Row * Col; i += Step)
            {
                auto b = RandomEngine::Platform::SIMD::LoadConvert<T, ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreConvert<T, ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Add<ResultType>(sv, b));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) + static_cast<ResultType>(rhs[i]);
        }
        else
        {
            for (size_t i = 0; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) + static_cast<ResultType>(rhs[i]);
        }
        return result;
    }

    template <Detail::NumericMat T, size_t Row, size_t Col, Detail::NumericMat U>
    constexpr auto operator-(const Mat<T, Row, Col> &lhs, U rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Mat<ResultType, Row, Col> result;
        if constexpr (Detail::MatUseSIMD<ResultType, Row, Col> && std::is_same_v<T, ResultType>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
            std::size_t i = 0;
            for (; i + W <= Row * Col; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(a, sv));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) - static_cast<ResultType>(rhs);
        }
        else if constexpr (Detail::CanConvertSIMD<T, ResultType> && Detail::MatUseSIMD<T, Row, Col>)
        {
            constexpr std::size_t Step = RandomEngine::Platform::SIMD::ConvertStride<T, ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
            std::size_t i = 0;
            for (; i + Step <= Row * Col; i += Step)
            {
                auto a = RandomEngine::Platform::SIMD::LoadConvert<T, ResultType>(&lhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreConvert<T, ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(a, sv));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) - static_cast<ResultType>(rhs);
        }
        else
        {
            for (size_t i = 0; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) - static_cast<ResultType>(rhs);
        }
        return result;
    }

    template <Detail::NumericMat T, size_t Row, size_t Col, Detail::NumericMat U>
    constexpr auto operator-(U lhs, const Mat<T, Row, Col> &rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Mat<ResultType, Row, Col> result;
        if constexpr (Detail::MatUseSIMD<ResultType, Row, Col> && std::is_same_v<T, ResultType>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
            std::size_t i = 0;
            for (; i + W <= Row * Col; i += W)
            {
                auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(sv, b));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) - static_cast<ResultType>(rhs[i]);
        }
        else if constexpr (Detail::CanConvertSIMD<T, ResultType> && Detail::MatUseSIMD<T, Row, Col>)
        {
            constexpr std::size_t Step = RandomEngine::Platform::SIMD::ConvertStride<T, ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
            std::size_t i = 0;
            for (; i + Step <= Row * Col; i += Step)
            {
                auto b = RandomEngine::Platform::SIMD::LoadConvert<T, ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreConvert<T, ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Sub<ResultType>(sv, b));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) - static_cast<ResultType>(rhs[i]);
        }
        else
        {
            for (size_t i = 0; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) - static_cast<ResultType>(rhs[i]);
        }
        return result;
    }

    template <Detail::NumericMat T, size_t Row, size_t Col, Detail::NumericMat U>
    constexpr auto operator*(const Mat<T, Row, Col> &lhs, U rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Mat<ResultType, Row, Col> result;
        if constexpr (Detail::MatUseSIMD<ResultType, Row, Col> && std::is_same_v<T, ResultType>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
            std::size_t i = 0;
            for (; i + W <= Row * Col; i += W)
            {
                auto a = RandomEngine::Platform::SIMD::LoadU<ResultType>(&lhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Mul<ResultType>(a, sv));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) * static_cast<ResultType>(rhs);
        }
        else if constexpr (Detail::CanConvertSIMD<T, ResultType> && Detail::MatUseSIMD<T, Row, Col>)
        {
            constexpr std::size_t Step = RandomEngine::Platform::SIMD::ConvertStride<T, ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(rhs));
            std::size_t i = 0;
            for (; i + Step <= Row * Col; i += Step)
            {
                auto a = RandomEngine::Platform::SIMD::LoadConvert<T, ResultType>(&lhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreConvert<T, ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Mul<ResultType>(a, sv));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) * static_cast<ResultType>(rhs);
        }
        else
        {
            for (size_t i = 0; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs[i]) * static_cast<ResultType>(rhs);
        }
        return result;
    }

    template <Detail::NumericMat T, size_t Row, size_t Col, Detail::NumericMat U>
    constexpr auto operator*(U lhs, const Mat<T, Row, Col> &rhs)
    {
        using ResultType = std::common_type_t<T, U>;
        Mat<ResultType, Row, Col> result;
        if constexpr (Detail::MatUseSIMD<ResultType, Row, Col> && std::is_same_v<T, ResultType>)
        {
            constexpr std::size_t W = RandomEngine::Platform::SIMD::SIMDWidth<ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
            std::size_t i = 0;
            for (; i + W <= Row * Col; i += W)
            {
                auto b = RandomEngine::Platform::SIMD::LoadU<ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreU<ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Mul<ResultType>(sv, b));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) * static_cast<ResultType>(rhs[i]);
        }
        else if constexpr (Detail::CanConvertSIMD<T, ResultType> && Detail::MatUseSIMD<T, Row, Col>)
        {
            constexpr std::size_t Step = RandomEngine::Platform::SIMD::ConvertStride<T, ResultType>;
            auto sv = RandomEngine::Platform::SIMD::Set1<ResultType>(static_cast<ResultType>(lhs));
            std::size_t i = 0;
            for (; i + Step <= Row * Col; i += Step)
            {
                auto b = RandomEngine::Platform::SIMD::LoadConvert<T, ResultType>(&rhs.m_data[i]);
                RandomEngine::Platform::SIMD::StoreConvert<T, ResultType>(&result.m_data[i], RandomEngine::Platform::SIMD::Mul<ResultType>(sv, b));
            }
            for (; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) * static_cast<ResultType>(rhs[i]);
        }
        else
        {
            for (size_t i = 0; i < Row * Col; ++i)
                result[i] = static_cast<ResultType>(lhs) * static_cast<ResultType>(rhs[i]);
        }
        return result;
    }

    // 常用矩阵类型
    using Mat2i = Mat<int, 2, 2>;
    using Mat2f = Mat<float, 2, 2>;
    using Mat2d = Mat<double, 2, 2>;
    using Mat2l = Mat<long, 2, 2>;
    using Mat3i = Mat<int, 3, 3>;
    using Mat3f = Mat<float, 3, 3>;
    using Mat3d = Mat<double, 3, 3>;
    using Mat3l = Mat<long, 3, 3>;
    using Mat4i = Mat<int, 4, 4>;
    using Mat4f = Mat<float, 4, 4>;
    using Mat4d = Mat<double, 4, 4>;
    using Mat4l = Mat<long, 4, 4>;

    // 矩阵视图
    template <Detail::NumericMat T, size_t Row, size_t Col, typename RowRange, typename ColRange>
    struct MatView final
    {
    private:
        Mat<T, Row, Col> &_original_mat;
        static constexpr auto _row_indices = RowRange::Values();
        static constexpr auto _col_indices = ColRange::Values();

    public:
        // 构造函数
        MatView(Mat<T, Row, Col> &mat, RowRange, ColRange) : _original_mat(mat) {}

        // 赋值
        template <Detail::NumericMat U>
        constexpr MatView &operator=(const Mat<U, RowRange::Size(), ColRange::Size()> &other)
        {
            for (size_t r = 0; r < RowRange::Size(); ++r)
            {
                for (size_t c = 0; c < ColRange::Size(); ++c)
                {
                    _original_mat[_row_indices[r], _col_indices[c]] = static_cast<T>(other[r, c]);
                }
            }
            return *this;
        }

        template <Detail::NumericMat U>
        constexpr MatView &operator=(const std::initializer_list<U> &ilist)
        {
            if (ilist.size() != RowRange::Size() * ColRange::Size())
            {
                throw std::runtime_error("Size mismatch");
            }
            size_t index = 0;
            for (const auto &value : ilist)
            {
                size_t r = index / ColRange::Size();
                size_t c = index % ColRange::Size();
                _original_mat[_row_indices[r], _col_indices[c]] = static_cast<T>(value);
                index++;
            }
            return *this;
        }

        template <Detail::NumericMat U>
        constexpr MatView &operator=(const std::array<U, RowRange::Size() * ColRange::Size()> &arr)
        {
            size_t index = 0;
            for (size_t r = 0; r < RowRange::Size(); ++r)
            {
                for (size_t c = 0; c < ColRange::Size(); ++c)
                {
                    _original_mat[_row_indices[r], _col_indices[c]] = static_cast<T>(arr[index++]);
                }
            }
            return *this;
        }

        template <Detail::NumericMat U>
        constexpr MatView &operator=(const std::vector<U> &vec)
        {
            if (vec.size() != RowRange::Size() * ColRange::Size())
            {
                throw std::runtime_error("Size mismatch");
            }
            size_t index = 0;
            for (size_t r = 0; r < RowRange::Size(); ++r)
            {
                for (size_t c = 0; c < ColRange::Size(); ++c)
                {
                    _original_mat[_row_indices[r], _col_indices[c]] = static_cast<T>(vec[index++]);
                }
            }
            return *this;
        }

        template <Detail::NumericMat U>
        constexpr MatView &operator=(const std::list<U> &list)
        {
            if (list.size() != RowRange::Size() * ColRange::Size())
            {
                throw std::runtime_error("Size mismatch");
            }
            size_t index = 0;
            for (const auto &value : list)
            {
                size_t r = index / ColRange::Size();
                size_t c = index % ColRange::Size();
                _original_mat[_row_indices[r], _col_indices[c]] = static_cast<T>(value);
                index++;
            }
            return *this;
        }

        template <Detail::NumericMat U>
        constexpr MatView &operator=(const std::span<U, RowRange::Size() * ColRange::Size()> &span)
        {
            size_t index = 0;
            for (size_t r = 0; r < RowRange::Size(); ++r)
            {
                for (size_t c = 0; c < ColRange::Size(); ++c)
                {
                    _original_mat[_row_indices[r], _col_indices[c]] = static_cast<T>(span[index++]);
                }
            }
            return *this;
        }

        //// 隐式转换回Mat
        template <Detail::NumericMat U>
        constexpr operator Mat<U, RowRange::Size(), ColRange::Size()>() const
        {
            Mat<U, RowRange::Size(), ColRange::Size()> result;
            for (size_t r = 0; r < RowRange::Size(); ++r)
            {
                for (size_t c = 0; c < ColRange::Size(); ++c)
                {
                    result[r, c] = static_cast<U>(_original_mat[_row_indices[r], _col_indices[c]]);
                }
            }
            return result;
        }
    };

}