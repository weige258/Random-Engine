#pragma once
#include <atomic>
#include <thread>

namespace RandomEngine::Core::Memory { 

// ══════════ 1. 可重入自旋锁(TTAS + pause + 伪共享隔离) ══════════
    struct alignas(64) IdLock {                  // 独占 cache line,消伪共享
        std::atomic<uint32_t> m_owner{0};        // 0=空闲, 否则=线程 token
        uint32_t m_depth = 0;                    // 重入深度(仅 owner 触碰,无需原子)

        void Lock(uint32_t token) noexcept {
            if (m_owner.load(std::memory_order_relaxed) == token) {   // 快路径:重入
                ++m_depth;
                return;
            }
            for (;;) {
                if (m_owner.load(std::memory_order_relaxed) == 0) {   // TTAS:只在空闲时才 CAS
                    uint32_t expected = 0;
                    if (m_owner.compare_exchange_weak(expected, token,
                            std::memory_order_acquire, std::memory_order_relaxed)) {
                        m_depth = 1;
                        return;
                    }
                }
                for (int i = 0; i < 64; ++i) {   // 就地自旋,不进内核
#if defined(_MSC_VER)
                    _mm_pause();
#else
                    __builtin_ia32_pause();
#endif
                }
                std::this_thread::yield();       // 长争用兜底,让出核
            }
        }
        void Unlock(uint32_t token) noexcept {
            if (--m_depth == 0)
                m_owner.store(0, std::memory_order_release);
        }
        bool IsHeldBy(uint32_t token) const noexcept {
            return m_owner.load(std::memory_order_relaxed) == token;
        }
    };

    inline uint32_t ThisThreadToken() noexcept {
        static std::atomic<uint32_t> s_next{1};
        thread_local const uint32_t s_token =
            s_next.fetch_add(1, std::memory_order_relaxed);
        return s_token;
    }
}