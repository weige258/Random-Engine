#pragma once
#include "JobWorker.hpp"
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <type_traits>

namespace RandEngine::Core::Job
{
    template <typename Task>
    class LoopWorker : public JobWorker
    {
    protected:
        std::vector<Task> m_dedicated_tasks;
        mutable std::mutex m_task_mutex;
        std::atomic<size_t> m_task_count{0};
        std::vector<Task> m_local_cache;

        static bool IsValid(const Task &task)
        {
            if constexpr (std::is_pointer_v<Task>)
                return task != nullptr;
            else
                return static_cast<bool>(task);
        }

    public:
        LoopWorker() = default;
        ~LoopWorker() override { Stop(); }

        // --- 基础任务容器操作 ---

        void PushTask(const Task &task)
        {
            if (!IsValid(task)) return;
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_dedicated_tasks.push_back(task);
            m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed);
            WakeUp();
        }

        bool RemoveTask(const Task &task)
        {
            if (!IsValid(task)) return false;
            std::lock_guard<std::mutex> lock(m_task_mutex);
            auto it = std::remove(m_dedicated_tasks.begin(), m_dedicated_tasks.end(), task);
            if (it != m_dedicated_tasks.end())
            {
                m_dedicated_tasks.erase(it, m_dedicated_tasks.end());
                m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed);
                return true;
            }
            return false;
        }

        [[nodiscard]] size_t GetTaskCount() const 
        { 
            return m_task_count.load(std::memory_order_relaxed); 
        }

        std::unique_lock<std::mutex> LockQueue() { return std::unique_lock<std::mutex>(m_task_mutex); }
        std::vector<Task>& GetRawTasks() { return m_dedicated_tasks; }
        void UpdateTaskCount() { m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed); }

    protected:
        // 单个 Task 的派生重载点：默认尝试调用 task.Execute()，派生类可随意覆写执行细节
        virtual void ExecuteTask(Task &task)
        {
            if constexpr (requires { task.Execute(); })
            {
                task.Execute();
            }
        }

        // 核心单步逻辑：仅做数据快照与顺序循环调用
        void ProcessWork() override
        {
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                m_local_cache.clear();

                bool has_invalid = false;
                for (const auto &task : m_dedicated_tasks)
                {
                    if (IsValid(task))
                        m_local_cache.push_back(task);
                    else
                        has_invalid = true;
                }

                if (has_invalid)
                {
                    m_dedicated_tasks = m_local_cache;
                    m_task_count.store(m_dedicated_tasks.size(), std::memory_order_relaxed);
                }
            }

            if (!m_local_cache.empty())
            {
                for (auto &task : m_local_cache)
                {
                    ExecuteTask(task);
                }
            }
            else
            {
                std::this_thread::yield();
            }
        }
    };
}