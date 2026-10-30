#pragma once
#include "BaseJobWorker.hpp"
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <type_traits>

namespace RandomEngine::Core::Jobs::JobWorker
{
    template <typename Job>
    class BaseLoopJobWorker : public BaseJobWorker
    {
    protected:
        std::vector<Job> m_dedicated_tasks;
        mutable std::mutex m_task_mutex;
        std::atomic<size_t> m_task_count{0};
        std::atomic<bool> m_dirty{true};
        std::vector<Job> m_local_cache;

        static bool IsValid(const Job &task)
        {
            if constexpr (std::is_pointer_v<Job>)
                return task != nullptr;
            else
                return static_cast<bool>(task);
        }

    public:
        BaseLoopJobWorker() = default;
        ~BaseLoopJobWorker() override { Stop(); }

        void PushTask(const Job &task)
        {
            if (!IsValid(task)) return;
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                m_dedicated_tasks.push_back(task);
                m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed);
                m_dirty.store(true, std::memory_order_relaxed);
            }
            std::lock_guard<std::mutex> lock(m_cv_mutex);
            m_cv.notify_one();
        }

        bool RemoveTask(const Job &task)
        {
            if (!IsValid(task)) return false;
            std::lock_guard<std::mutex> lock(m_task_mutex);
            auto it = std::remove(m_dedicated_tasks.begin(), m_dedicated_tasks.end(), task);
            if (it != m_dedicated_tasks.end())
            {
                m_dedicated_tasks.erase(it, m_dedicated_tasks.end());
                m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed);
                m_dirty.store(true, std::memory_order_relaxed);
                return true;
            }
            return false;
        }

        [[nodiscard]] size_t GetTaskCount() const
        {
            return m_task_count.load(std::memory_order_relaxed);
        }

        std::unique_lock<std::mutex> LockQueue() { return std::unique_lock<std::mutex>(m_task_mutex); }
        std::vector<Job>& GetRawTasks() { return m_dedicated_tasks; }
        void UpdateTaskCount() { m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed); }

        // 由外部（如 Executor 的均衡迁移）在持有 m_task_mutex 修改 m_dedicated_tasks 后调用，
        // 通知 Worker 在下一轮 ProcessWork 中重新刷新 m_local_cache。
        // 必须唤醒：Worker 在缓存为空时会阻塞等待，只置 dirty 而不唤醒会让它永久睡死。
        void MarkDirty()
        {
            m_dirty.store(true, std::memory_order_relaxed);
            WakeUp();
        }

    protected:
        // 唤醒挂点：默认唤醒自身的 m_cv。
        // 重写 ProcessWork 并改睡在其他条件变量上的子类（如 FixUpdateJobWorker 睡在 Stepper 的 cv）
        // 必须覆盖此函数，否则 MarkDirty 对它们完全无效。
        virtual void WakeUp()
        {
            std::lock_guard<std::mutex> lock(m_cv_mutex);
            m_cv.notify_one();
        }

        virtual void ExecuteJob(Job &job)
        {
            if constexpr (requires { { job.Execute() } -> std::same_as<void>; })
            {
                job.Execute();
            }
        }

        void ProcessWork() override
        {
            if (m_dirty.exchange(false, std::memory_order_acquire))
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                m_local_cache = m_dedicated_tasks;
            }

            if (!m_local_cache.empty())
            {
                for (auto &task : m_local_cache)
                {
                    ExecuteJob(task);
                }
            }
            else
            {
                std::unique_lock<std::mutex> lock(m_cv_mutex);
                m_cv.wait(lock, [this]()
                {
                    return m_dirty.load(std::memory_order_relaxed) || m_is_paused.load(std::memory_order_relaxed) || !m_is_running.load(std::memory_order_relaxed);
                });
            }
        }
    };
}