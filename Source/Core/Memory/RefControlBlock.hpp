#pragma once
#include <atomic>
#include <cstdint>

namespace RandomEngine::Core::Memory
{
    struct RefControlBlock;
    struct RefDomain;

    struct RefVisitor
    {
        void* ctx = nullptr;
        void (*edge)(void*, RefControlBlock*) noexcept = nullptr;
        void Edge(RefControlBlock* to) const noexcept { if (edge) edge(ctx, to); }
    };

    // 外置控制块：计数不内嵌对象，块也不属于任何全局容器
    //   bit 63 ClosingBit / bits 32..62 Strong / bits 0..31 Reader
    struct RefControlBlock
    {
        static constexpr std::uint64_t ClosingBit = 1ull << 63;
        static constexpr std::uint64_t StrongUnit = 1ull << 32;
        static constexpr std::uint64_t StrongMask = 0x7FFFFFFFull << 32;
        static constexpr std::uint64_t ReaderMask = 0xFFFFFFFFull;

        std::atomic<std::uint64_t> state{0};
        std::atomic<bool>          reaped{false};

        void* target  = nullptr;
        void (*destroy)(void*) noexcept = nullptr;
        void (*trace)(void*, RefVisitor&) noexcept = nullptr;

        RefDomain*       domain   = nullptr;   // 侵入链表，无 vector / 无 unique_ptr 全表
        RefControlBlock* next     = nullptr;
        bool             orphaned = false;     // 域已析构：块由最后一次 Release/Leave 自删

        std::uint32_t gc_refs = 0;             // 只在域析构的 Reclaim 里读写
        std::uint8_t  mark    = 0;
        bool          in_S    = false;

        std::uint32_t StrongCount() const noexcept {
            return static_cast<std::uint32_t>(
                (state.load(std::memory_order_acquire) & StrongMask) >> 32);
        }
        bool Closed()    const noexcept { return (state.load(std::memory_order_acquire) & ClosingBit) != 0; }
        bool Reaped()    const noexcept { return reaped.load(std::memory_order_acquire); }
        bool HasReader() const noexcept { return (state.load(std::memory_order_acquire) & ReaderMask) != 0; }

        void AddRef() noexcept { state.fetch_add(StrongUnit, std::memory_order_relaxed); }

        void Release() noexcept {
            const std::uint64_t old = state.fetch_sub(StrongUnit, std::memory_order_acq_rel);
            if (((old & StrongMask) >> 32) == 1) { Close(); TryDestroyNow(); }
            MaybeFreeBlock();
        }

        void Close() noexcept { state.fetch_or(ClosingBit, std::memory_order_acq_rel); } // O(1)，不等人

        bool TryEnter() noexcept {
            std::uint64_t s = state.load(std::memory_order_acquire);
            for (;;) {
                if (s & ClosingBit) return false;
                if ((s & ReaderMask) == ReaderMask) return false;
                if (state.compare_exchange_weak(s, s + 1,
                        std::memory_order_acquire, std::memory_order_relaxed)) return true;
            }
        }

        void Leave() noexcept {
            state.fetch_sub(1, std::memory_order_acq_rel);
            TryDestroyNow();
            MaybeFreeBlock();
        }

        void TryDestroyNow() noexcept {
            const std::uint64_t s = state.load(std::memory_order_acquire);
            if (!(s & ClosingBit) || (s & ReaderMask) != 0) return;   // 不打断执行者
            if (reaped.exchange(true, std::memory_order_acq_rel)) return;
            void* p = target;
            target  = nullptr;
            if (destroy) destroy(p);
        }

        void MaybeFreeBlock() noexcept {
            if (!orphaned) return;
            if (StrongCount() != 0 || HasReader() || !Reaped() || !Closed()) return;
            delete this;
        }

        void Trace(RefVisitor& v) noexcept { if (trace && target) trace(target, v); }
    };

    // 叶子默认无出边；参与环回收的类型在自己命名空间写非模板重载，ADL 命中
    template <typename T>
    inline void TraceEdges(const T&, RefVisitor&) noexcept {}
}