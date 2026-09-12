#pragma once
#include <atomic>
#include <thread>
#include <algorithm>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace RandomEngine::Core::Memory {

// 跨平台 CPU Pause 指令
inline void CpuPause() noexcept {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    _mm_pause();
#elif defined(__i386__) || defined(__x86_64__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(_M_ARM64)
    asm volatile("yield" ::: "memory");
#else
    // 其它架构兜底
#endif
}

struct alignas(64) IdLock {
    private:
    
    uint32_t m_owner = 0;
    uint32_t m_depth = 0;

    public:
    void Lock(uint32_t token) noexcept {
        std::atomic_ref<uint32_t> owner(m_owner);

        if (owner.load(std::memory_order_relaxed) == token) {
            ++m_depth;
            return;
        }

        uint32_t backoff = 1;
        for (;;) {
            if (owner.load(std::memory_order_relaxed) == 0) {
                uint32_t expected = 0;
                if (owner.compare_exchange_weak(expected, token,
                        std::memory_order_acquire,
                        std::memory_order_relaxed)) {
                    m_depth = 1;
                    return;
                }
            }

            // 指数退避，避免 12700H 等新架构下 64 次 pause 导致耗时过长
            for (uint32_t i = 0; i < backoff; ++i) {
                CpuPause();
            }
            backoff = std::min(backoff << 1, 16u);

            if (backoff >= 16u) {
                std::this_thread::yield();
            }
        }
    }

    void Unlock(uint32_t token) noexcept {
        if (--m_depth == 0) {
            std::atomic_ref<uint32_t>(m_owner)
                .store(0, std::memory_order_release);
        }
    }

    bool IsHeldBy(uint32_t token) const noexcept {
        return std::atomic_ref<const uint32_t>(m_owner)
            .load(std::memory_order_relaxed) == token;
    }
};

static_assert(std::is_trivially_copyable_v<IdLock>,
              "IdLock 必须 trivially copyable，以支持 vector 预分配初始化");

inline uint32_t ThisThreadToken() noexcept {
    static std::atomic<uint32_t> s_next{1};
    thread_local const uint32_t s_token = s_next.fetch_add(1, std::memory_order_relaxed);
    return s_token;
}

} // namespace RandomEngine::Core::Memory