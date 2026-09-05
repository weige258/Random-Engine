#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace RandomEngine::Core::Jobs::JobWorker
{

    class BaseJobWorker
    {
    protected:
        std::thread m_thread;
        std::atomic<bool> m_is_running{false};
        std::atomic<bool> m_is_paused{false};

        std::mutex m_cv_mutex;
        std::condition_variable m_cv;

    public:
        BaseJobWorker() = default;
        virtual ~BaseJobWorker()
        {
            if (m_thread.joinable())
            {
                Stop();
            }
        }

        BaseJobWorker(const BaseJobWorker &) = delete;
        BaseJobWorker &operator=(const BaseJobWorker &) = delete;

        void Start()
        {
            if (m_is_running.exchange(true, std::memory_order_acq_rel))
                return;

            m_is_paused.store(false, std::memory_order_release);
            m_thread = std::thread([this]()
                                   { ThreadLoop(); });
        }

        void Stop()
        {
            if (!m_is_running.exchange(false, std::memory_order_acq_rel))
                return;

            {
                std::lock_guard<std::mutex> lock(m_cv_mutex);
                m_cv.notify_one();
            }
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        void Pause()
        {
            m_is_paused.store(true, std::memory_order_release);
        }

        void Resume()
        {
            if (m_is_paused.exchange(false, std::memory_order_acq_rel))
            {
                std::lock_guard<std::mutex> lock(m_cv_mutex);
                m_cv.notify_one();
            }
        }

        [[nodiscard]] bool IsRunning() const { return m_is_running.load(std::memory_order_relaxed); }
        [[nodiscard]] bool IsPaused() const { return m_is_paused.load(std::memory_order_relaxed); }

    protected:
        virtual void OnStart() {}
        virtual void OnStop() {}
        virtual void OnLoopStart() {}
        virtual void ProcessWork() = 0;
        virtual void OnLoopEnd() {}

    private:
        void ThreadLoop()
        {
            OnStart();

            while (m_is_running.load(std::memory_order_relaxed))
            {
                if (m_is_paused.load(std::memory_order_relaxed))
                {
                    std::unique_lock<std::mutex> lock(m_cv_mutex);
                    m_cv.wait(lock, [this]()
                              { return !m_is_running.load(std::memory_order_relaxed) ||
                                       !m_is_paused.load(std::memory_order_relaxed); });

                    if (!m_is_running.load(std::memory_order_relaxed))
                        break;
                }

                OnLoopStart();
                ProcessWork();
                OnLoopEnd();
            }

            OnStop();
        }
    };

}