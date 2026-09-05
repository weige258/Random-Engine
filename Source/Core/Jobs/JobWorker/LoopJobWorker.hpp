#pragma once
#include "BaseJobWorker.hpp"
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <type_traits>
#include <chrono>

namespace RandomEngine::Core::Jobs::JobWorker
{
    template <typename Job>
    class LoopJobWorker : public BaseJobWorker
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
        LoopJobWorker() = default;
        ~LoopJobWorker() override { Stop(); }

        void PushTask(const Job &task)
        {
            if (!IsValid(task)) return;
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_dedicated_tasks.push_back(task);
            m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed);
            m_dirty.store(true, std::memory_order_relaxed);
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

    protected:
        virtual void ExecuteJob(Job &job)
        {
            if constexpr (requires { job.Execute(); })
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
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    };
}