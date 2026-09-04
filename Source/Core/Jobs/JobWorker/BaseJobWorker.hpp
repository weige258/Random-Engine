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

        // 线程挂起与唤醒的条件变量
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

        // 启动 Worker 线程
        void Start()
        {
            if (m_is_running.exchange(true, std::memory_order_acq_rel))
                return;

            m_is_paused.store(false, std::memory_order_release);
            m_thread = std::thread([this]()
                                   { ThreadLoop(); });
        }

        // 安全停止 Worker 线程
        void Stop()
        {
            if (!m_is_running.exchange(false, std::memory_order_acq_rel))
                return;

            WakeUp(); // 唤醒可能处于休眠/暂停状态的线程以安全退栈
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        // 暂停线程执行
        void Pause()
        {
            m_is_paused.store(true, std::memory_order_release);
        }

        // 恢复线程执行
        void Resume()
        {
            if (m_is_paused.exchange(false, std::memory_order_acq_rel))
            {
                WakeUp();
            }
        }

        // 主动唤醒挂起的 Worker 线程（例如外部 Push 任务时调用）
        void WakeUp()
        {
            std::lock_guard<std::mutex> lock(m_cv_mutex);
            m_cv.notify_one();
        }

        [[nodiscard]] bool IsRunning() const { return m_is_running.load(std::memory_order_relaxed); }
        [[nodiscard]] bool IsPaused() const { return m_is_paused.load(std::memory_order_relaxed); }

    protected:
        // --- 派生类可重载的生命周期与循环钩子 (Virtual Hooks) ---

        // 1. 线程创建启动时触发一次
        virtual void OnStart() {}

        // 2. 线程退出销毁前触发一次
        virtual void OnStop() {}

        // 3. 每轮循环开始前触发（如 LoopWorker 在此计算 DeltaTime、QueueWorker 在此抓取队列快照）
        virtual void OnLoopStart() {}

        // 4. 核心工作单步逻辑（由子类实现具体任务执行）
        virtual void ProcessWork() = 0;

        // 5. 每轮循环结束后触发
        virtual void OnLoopEnd() {}

        // 派生类辅助工具：队列无任务时挂起线程，防止 CPU 无意义 Spin
        template <typename Predicate>
        void WaitIfIdle(Predicate &&has_work_predicate)
        {
            std::unique_lock<std::mutex> lock(m_cv_mutex);
            m_cv.wait(lock, [this, &has_work_predicate]()
                      { return !m_is_running.load(std::memory_order_relaxed) ||
                               (!m_is_paused.load(std::memory_order_relaxed) && has_work_predicate()); });
        }

    private:
        void ThreadLoop()
        {
            OnStart();

            while (m_is_running.load(std::memory_order_relaxed))
            {
                // 处理暂停逻辑
                if (m_is_paused.load(std::memory_order_relaxed))
                {
                    std::unique_lock<std::mutex> lock(m_cv_mutex);
                    m_cv.wait(lock, [this]()
                              { return !m_is_running.load(std::memory_order_relaxed) ||
                                       !m_is_paused.load(std::memory_order_relaxed); });

                    if (!m_is_running.load(std::memory_order_relaxed))
                        break;
                }

                // 1. 触发轮次开始钩子（可在此计算 dt）
                OnLoopStart();

                // 2. 执行具体业务
                ProcessWork();

                // 3. 触发轮次结束钩子
                OnLoopEnd();
            }

            OnStop();
        }
    };

}
