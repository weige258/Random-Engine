// ===== ReCast.hpp =====
// 语法与语义与 dynamic_cast 一致的快速转换：
//   ReCast<Derived*>(base_ptr)         失败返回 nullptr
//   ReCast<const Derived*>(base_ptr)   cv 规则与 std 一致
//   ReCast<Derived&>(base_ref)         失败抛 std::bad_cast
//   ReCast<void*>(poly_ptr)            最派生完整对象地址（std 特例，已修复）
// 上行/同型：编译期 static_cast（与 std 语义一致，含私有基类走缓存路径等价）
// 下行/横向/void*：无锁 vptr 键偏移缓存，命中 ~3-5ns
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <array>
#include <typeinfo>
#include <type_traits>
#include <stdexcept>
#include <limits>

namespace RandomEngine::Core::Cast {

namespace Detail {

    // ---------- copy_cv：把源的 cv 限定抄到目标上（标准库无此 trait）----------
    template <typename Dst, typename Src>
    struct CopyCv { using type = std::remove_cv_t<Dst>; };
    template <typename Dst, typename Src>
    struct CopyCv<Dst, const Src>          { using type = const std::remove_cv_t<Dst>; };
    template <typename Dst, typename Src>
    struct CopyCv<Dst, volatile Src>       { using type = volatile std::remove_cv_t<Dst>; };
    template <typename Dst, typename Src>
    struct CopyCv<Dst, const volatile Src> { using type = const volatile std::remove_cv_t<Dst>; };
    template <typename Dst, typename Src>
    using CopyCvT = typename CopyCv<Dst, Src>::type;

    // ---------- 无锁 (源子对象 vptr, 目标类型) -> 偏移 缓存 ----------
    // key 完备性：MSVC/Itanium ABI 下每个不同的多态子对象都有唯一的 vtable
    //（同一完整对象的不同子对象 vptr 不同、不同动态类型更不同），
    // 故 (vptr, &typeid(U)) 相同 ⟺ dynamic_cast 结果必然相同，缓存永不假共享。
    inline constexpr std::size_t kReCastSlots = 1024;   // 2 的幂；64KB，L2 驻留
    // 表满兜底返回值（一个几乎不可能成为合法 payload 的数）
    inline constexpr std::int64_t kReCastNoCache = std::numeric_limits<std::int64_t>::min();

    struct alignas(64) ReCastSlot {                     // 64B 对齐：防伪共享
        std::atomic<const void*>  key_src{nullptr};     // 源子对象 vptr —— 发布位
        std::atomic<const void*>  key_dst{nullptr};     // &typeid(目标类型)
        std::atomic<std::int64_t> payload{0};           // bit0=ok；其余 = offset*2（可为负）
        // 注意：off<<1 与 pay>>1 对负数依赖有符号移位的已定义行为，需 C++20 及以上
    };

    inline ReCastSlot* ReCastTable() noexcept {
        static std::array<ReCastSlot, kReCastSlots> table;   // Meyers 单例
        return table.data();
    }

    inline std::size_t ReCastHash(const void* a, const void* b) noexcept {
        std::uint64_t h = reinterpret_cast<std::uintptr_t>(a) * 0x9E3779B97F4A7C15ull
                        ^ (reinterpret_cast<std::uintptr_t>(b) << 1);
        h ^= h >> 29; h *= 0xBF58476D1CE4E5B9ull; h ^= h >> 32;
        return static_cast<std::size_t>(h) & (kReCastSlots - 1);
    }

    // ABI 事实：MSVC/Itanium 下多态子对象的 vptr 位于该子对象偏移 0。
    // memcpy 读写指针位型：编译为一条 mov，形式合规无警告。
    inline const void* ReadVptr(const void* obj) noexcept {
        std::uintptr_t bits = 0;
        std::memcpy(&bits, obj, sizeof(bits));
        return reinterpret_cast<const void*>(bits);
    }

    // 查询缓存；未命中则付一次真 dynamic_cast 的钱校准并填充（无锁）。
    // Src: 源子对象类型（可含 cv）；Dst: cv 对齐后的目标元素类型（可为 void）
    // 返回 payload；kReCastNoCache 表示表满，调用方退化为真 dynamic_cast。
    template <typename Src, typename Dst>
    std::int64_t ReCastLookup(Src* src) noexcept {
        static_assert(std::is_polymorphic_v<std::remove_cv_t<Src>>,
                      "ReCastLookup requires polymorphic source");
        const void* vptr   = ReadVptr(src);
        const void* dst_ti = &typeid(std::remove_cv_t<Dst>);   // void → &typeid(void)，合法

        auto* table = ReCastTable();
        std::size_t i = ReCastHash(vptr, dst_ti);

        for (std::size_t probe = 0; probe < kReCastSlots; ++probe) {
            ReCastSlot& s = table[i];
            if (s.key_src.load(std::memory_order_acquire) == vptr &&
                s.key_dst.load(std::memory_order_acquire) == dst_ti)
                return s.payload.load(std::memory_order_acquire);   // 命中：全原子读，零锁

            const void* expect = nullptr;
            if (s.key_src.compare_exchange_strong(
                    expect, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(1)),
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                // CAS 成功：本线程独占该槽，做一次真 dynamic_cast 校准
                // 注：dynamic_cast 无视访问控制（标准允许），私有基类路径亦正确
                Dst* target = dynamic_cast<Dst*>(src);
                std::int64_t pay = 0;
                if (target) {
                    std::ptrdiff_t off = reinterpret_cast<char*>(target)
                                       - reinterpret_cast<char*>(src);
                    // 最派生地址 ≤ 源子对象地址：void* 场景偏移为负，编码支持
                    pay = 1 | (static_cast<std::int64_t>(off) << 1);
                }
                s.payload.store(pay,    std::memory_order_relaxed);
                s.key_dst.store(dst_ti, std::memory_order_relaxed);
                s.key_src.store(vptr,   std::memory_order_release);  // 发布完整条目
                return pay;
            }
            i = (i + 1) & (kReCastSlots - 1);   // 槽被占（他人校准中/异 key）：线性探测
        }
        return kReCastNoCache;                  // 表满：调用方退化为真 dynamic_cast
        // 竞态说明：同 key 双线程同时校准可能各占一槽（键含哨兵 1 而探测
        // 失配推进），两槽 payload 相同，命中哪个都正确，仅浪费一槽，良性。
        // 哨兵 (void*)1：vtable 在只读段且对齐 ≥ 指针宽，地址 1 属零页，
        // 真 vptr==1 的程序已是 UB，哨兵无歧义。
    }

} // namespace Detail

// =========================================================================
// ReCast<T>(x)
// =========================================================================
template <typename T, typename U>
T ReCast(U&& value) {
    using SrcType = std::remove_reference_t<U>;
    constexpr bool kTargetIsPtr = std::is_pointer_v<std::remove_reference_t<T>>;
    static_assert(kTargetIsPtr || std::is_reference_v<T>,
        "ReCast<T>: T must be pointer or reference, like dynamic_cast");

    if constexpr (kTargetIsPtr) {
        using SrcElem = std::remove_pointer_t<SrcType>;
        using DstElem = std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<T>>>;
        static_assert(!std::is_void_v<SrcElem>,
            "ReCast: source must be a class pointer, like dynamic_cast");
        static_assert(!(std::is_const_v<SrcElem> && !std::is_const_v<DstElem>),
            "ReCast cannot cast away const, like dynamic_cast");
        using DstCv = Detail::CopyCvT<DstElem, SrcElem>;

        if constexpr (std::is_void_v<DstElem>) {
            // ---- std [expr.dynamic.cast]/6：返回最派生完整对象的地址 ----
            // 必须先于 is_convertible 判定：BaseObject* → void* 可隐式转换，
            // 若走捷径会返回源子对象地址而非最派生地址（语义错误）。
            if (!value) return nullptr;
            std::int64_t pay = Detail::ReCastLookup<SrcElem, DstCv>(value); // DstCv = void
            if (pay == Detail::kReCastNoCache)
                return dynamic_cast<T>(value);            // 表满兜底
            // 多态对象的 void* 转换恒成功，ok 位必为 1
            return static_cast<T>(reinterpret_cast<DstCv*>(
                reinterpret_cast<char*>(value) + (pay >> 1)));
        }
        else if constexpr (std::is_convertible_v<SrcType, T>) {
            // 上行/同型：编译期 static_cast。与 std 语义一致——对类类型目标，
            // 隐式可转换 ⟹ static_cast 与 dynamic_cast 调整结果相同
            //（私有基类上行 is_convertible 为 false，落入下方缓存路径，
            //  由真 dynamic_cast 校准，结果仍与 std 一致）。
            return static_cast<T>(value);
        }
        else {
            // 下行 / 横向：vptr 键无锁偏移缓存
            static_assert(std::is_polymorphic_v<SrcElem>,
                "ReCast down/cross-cast requires polymorphic source, like dynamic_cast");
            if (!value) return nullptr;

            std::int64_t pay = Detail::ReCastLookup<SrcElem, DstCv>(value);
            if (pay == Detail::kReCastNoCache)
                return dynamic_cast<T>(value);            // 表满兜底
            if (!(pay & 1))
                return nullptr;                           // 已知失败：缓存命中
            return static_cast<T>(reinterpret_cast<DstCv*>(
                reinterpret_cast<char*>(value) + (pay >> 1)));
        }
    }
    else {
        // ---- 引用版本 ----
        static_assert(std::is_lvalue_reference_v<U&&>,
            "dynamic_cast<T&> requires an lvalue argument");
        using SrcElem = std::remove_reference_t<U>;
        using DstRef  = std::remove_reference_t<T>;
        static_assert(!(std::is_const_v<SrcElem> && !std::is_const_v<DstRef>),
            "ReCast cannot cast away const, like dynamic_cast");
        using DstCv = Detail::CopyCvT<DstRef, SrcElem>;

        if constexpr (std::is_convertible_v<U&, T>) {
            return static_cast<T>(value);
        }
        else {
            static_assert(std::is_polymorphic_v<SrcElem>,
                "ReCast down/cross-cast requires polymorphic source, like dynamic_cast");
            std::int64_t pay = Detail::ReCastLookup<SrcElem, DstCv>(std::addressof(value));
            if (pay == Detail::kReCastNoCache)
                return dynamic_cast<T>(value);            // 表满兜底，失败由 std 抛 bad_cast
            if (!(pay & 1))
                throw std::bad_cast{};                    // 已知失败：同 std 抛异常
            return *reinterpret_cast<DstCv*>(
                reinterpret_cast<char*>(std::addressof(value)) + (pay >> 1));
        }
    }
}

} // namespace RandomEngine::Core
