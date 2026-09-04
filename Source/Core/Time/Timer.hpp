#pragma once
#include <chrono>
#include <atomic>

namespace RandomEngine::Core::Time
{

    struct Timer final
    {

    private:
        using Clock = std::chrono::steady_clock;

        std::atomic<int64_t> m_start_time_ns{0};
        std::atomic<int64_t> m_last_check_ns{0};
        std::atomic<int64_t> m_paused_accum_ns{0};
        std::atomic<int64_t> m_pause_start_ns{0};
        std::atomic<float> m_time_scale{1.0f};
        std::atomic<bool> m_is_running{false};

    public:
        Timer() = default;
        ~Timer() = default;

        void Start()
        {
            int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
            bool expected = false;
            if (m_is_running.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            {
                int64_t pause_start = m_pause_start_ns.load(std::memory_order_relaxed);
                if (pause_start != 0)
                {
                    // 恢复播放：累加暂停期间消耗的时间
                    m_paused_accum_ns.fetch_add(now - pause_start, std::memory_order_relaxed);
                    m_pause_start_ns.store(0, std::memory_order_relaxed);
                }
                else
                {
                    // 首次启动
                    m_start_time_ns.store(now, std::memory_order_relaxed);
                    m_paused_accum_ns.store(0, std::memory_order_relaxed);
                }
                m_last_check_ns.store(now, std::memory_order_relaxed);
            }
        }

        void Stop()
        {
            bool expected = true;
            if (m_is_running.compare_exchange_strong(expected, false, std::memory_order_acq_rel))
            {
                m_pause_start_ns.store(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count(), std::memory_order_relaxed);
            }
        }

        float GetDeltaTime()
        {
            if (!m_is_running.load(std::memory_order_relaxed))
                return 0.0f;

            int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
            // 原子替换：获取上次采样时间并将 last_check 置为当前时间
            int64_t last = m_last_check_ns.exchange(now, std::memory_order_relaxed);

            double dt_sec = static_cast<double>(now - last) * 1e-9;
            return static_cast<float>(dt_sec) * m_time_scale.load(std::memory_order_relaxed);
        }

        float GetElapsedTime() const
        {
            if (m_start_time_ns.load(std::memory_order_relaxed) == 0)
                return 0.0f;

            bool running = m_is_running.load(std::memory_order_relaxed);
            int64_t now = running ? std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count()
                                  : m_pause_start_ns.load(std::memory_order_relaxed);
            int64_t start = m_start_time_ns.load(std::memory_order_relaxed);
            int64_t paused = m_paused_accum_ns.load(std::memory_order_relaxed);

            double total_sec = static_cast<double>(now - start - paused) * 1e-9;
            return static_cast<float>(total_sec) * m_time_scale.load(std::memory_order_relaxed);
        }

        void SetTimeScale(float scale)
        {
            m_time_scale.store(scale, std::memory_order_relaxed);
        }
    };
}