#pragma once

#include <atomic>
#include <cstdint>
#include <exception>
#include <utility>

namespace RandomEngine::Core::Memory
{
    struct PtrControlBlock
    {
        // 单原子状态布局：
        //   bit 63      : ClosingBit  入口已关闭，禁止新读者
        //   bits 32..62 : OwnerRef    MasterPtr + 每个 ObserverPtr 各一份
        //   bits  0..31 : Reader      每个 ScopedRef 各一份（TryEnter 成功即 +1）
        //
        // 所有"保活引用"都在同一个 64 位原子里：
        //   · ScopedRef 不需要额外引用计数原子操作 —— Reader 位本身就是它的保活引用
        //   · 控制块 delete 只在某次原子减操作把 AliveMask 减到 0 时发生，判定是原子的
        //     → 不可能出现"一个线程 delete、另一个线程还在访问"
        //   · ObserverPtr::Lock() 的原子操作数与原版完全相同，零额外开销
        static constexpr uint64_t ClosingBit = 1ull << 63;
        static constexpr uint64_t OwnerUnit  = 1ull << 32;
        static constexpr uint64_t OwnerMask  = 0x7FFFFFFF00000000ull;
        static constexpr uint64_t ReaderMask = 0x00000000FFFFFFFFull;
        static constexpr uint64_t AliveMask  = OwnerMask | ReaderMask;

        std::atomic<uint64_t> state{0};        // MasterPtr 构造时写入 OwnerUnit
        std::atomic<void*>    target_ptr{nullptr};

        void* owned_ptr = nullptr;
        void (*destroy_owned)(void*) noexcept = nullptr;

        // 对象销毁权认领：Close() 与最后一个 Leave() 可能同时到达，只让一个赢
        std::atomic<bool> reaped{false};

        void AddOwnerRef() noexcept
        {
            const uint64_t old = state.fetch_add(OwnerUnit, std::memory_order_relaxed);

            // OwnerRef 只有 31 位，再 +1 会进位到 bit63 伪造 ClosingBit。
            // 复用 fetch_add 的返回值做溢出哨兵，零额外原子开销。
            if ((old & OwnerMask) == OwnerMask)
                std::terminate();
        }

        void ReleaseOwnerRef() noexcept
        {
            const uint64_t old = state.fetch_sub(OwnerUnit, std::memory_order_acq_rel);
            if ((old & AliveMask) == OwnerUnit)   // 减之前恰好只剩这一份、且无读者
                delete this;
        }

        [[nodiscard]] bool TryEnter() noexcept
        {
            uint64_t current = state.load(std::memory_order_acquire);
            for (;;)
            {
                if (current & ClosingBit) return false;
                if ((current & ReaderMask) == ReaderMask) return false;
                if (state.compare_exchange_weak(current, current + 1,
                        std::memory_order_acq_rel, std::memory_order_acquire))
                    return true;
            }
        }

        void Leave() noexcept
        {
            const uint64_t old = state.fetch_sub(1, std::memory_order_acq_rel);
            if ((old & ReaderMask) == 0) std::terminate();

            // 最后一个读者且入口已关闭 → 由我销毁对象
            if ((old & ReaderMask) == 1 && (old & ClosingBit))
                TryDestroyNow();

            // 最后才承担控制块释放：减之前 owner=0 且 reader=1
            // 这保证 ScopedRef 比 ObserverPtr 活得长时也安全
            if ((old & AliveMask) == 1)
                delete this;              // 此后绝不再访问任何成员
        }

        // 关闭入口，立即返回，不等待任何读者
        void Close() noexcept
        {
            const uint64_t old = state.fetch_or(ClosingBit, std::memory_order_acq_rel);
            target_ptr.store(nullptr, std::memory_order_release);

            // ClosingBit 已设 → TryEnter 必失败 → 不可能再有新读者
            if ((old & ReaderMask) == 0)
                TryDestroyNow();          // 此刻无读者 → 删除者自己销毁
            // Close 不减 AliveMask，Master 那份 OwnerRef 仍在 → 这里绝不会 delete this
        }

        void CloseAndWait() noexcept { Close(); }   // 旧名保留，语义=不等待

    private:
        void TryDestroyNow() noexcept
        {
            bool expected = false;
            if (!reaped.compare_exchange_strong(expected, true,
                    std::memory_order_acq_rel, std::memory_order_acquire))
                return;

            void* obj = std::exchange(owned_ptr, nullptr);
            auto  del = std::exchange(destroy_owned, nullptr);
            if (obj && del) del(obj);    // 物理销毁：此刻 readers 必为 0

            // 绝不碰 state、绝不 delete this。
            // 控制块释放完全由各持有者自己的原子减操作决定。
        }
    };
}