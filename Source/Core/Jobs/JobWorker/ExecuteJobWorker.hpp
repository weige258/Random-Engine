#pragma once

#include "BaseJobWorker.hpp"

#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <type_traits>
#include <functional>

namespace RandomEngine::Core::Jobs::JobWorker
{
    template <typename Job>
    class ExecuteJobWorker : public BaseJobWorker
    {
    protected:
        std::deque<Job> m_task_queue;
        mutable std::mutex m_task_mutex;
        std::atomic<size_t> m_task_count{0};

        static bool IsValid(const Job &task)
        {
            if constexpr (std::is_pointer_v<Job>)
                return task != nullptr;
            else
                return static_cast<bool>(task);
        }

    public:
        ExecuteJobWorker() = default;
        ~ExecuteJobWorker() override { Stop(); }

        void PushTask(const Job &task)
        {
            if (!IsValid(task)) return;
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.push_back(task);
            m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
        }

        void PushTask(Job &&task)
        {
            if (!IsValid(task)) return;
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.push_back(std::move(task));
            m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
        }

        void PushTaskFront(Job &&task)
        {
            if (!IsValid(task)) return;
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.push_front(std::move(task));
            m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
        }

        void ClearTasks()
        {
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.clear();
            m_task_count.store(0, std::memory_order_relaxed);
        }

        [[nodiscard]] size_t GetTaskCount() const
        {
            return m_task_count.load(std::memory_order_relaxed);
        }

        [[nodiscard]] bool HasTask() const { return GetTaskCount() > 0; }

        std::unique_lock<std::mutex> LockQueue()
        {
            return std::unique_lock<std::mutex>(m_task_mutex);
        }

        std::deque<Job> &GetRawTasks() { return m_task_queue; }

        void StopAfterDrain()
        {
            while (m_is_running.load(std::memory_order_relaxed) && GetTaskCount() > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            Stop();
        }

    protected:
        virtual void ExecuteTask(Job &task)
        {
            if constexpr (requires { task.Execute(); })
            {
                task.Execute();
            }
            else if constexpr (std::is_invocable_v<Job &>)
            {
                task();
            }
        }

        void ProcessWork() override
        {
            Job task;
            bool has_task = false;

            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                while (!m_task_queue.empty())
                {
                    task = std::move(m_task_queue.front());
                    m_task_queue.pop_front();
                    m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
                    if (IsValid(task)) { has_task = true; break; }
                }
            }

            if (has_task)
            {
                ExecuteTask(task);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    };
}