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
    /// ExecuteJobWorker：一次性命令队列 Worker
    ///  - 外部 PushTask() 加入命令
    ///  - 内部线程按 FIFO 顺序：取出 -> 执行 -> 弹出（执行完自动移除）
    ///  - 队列空闲时线程挂起，PushTask / Stop 时自动唤醒
    template <typename Job>
    class ExecuteJobWorker : public JobWorker   // 若工程中基类名为 BaseJobWorker，请对应替换
    {
    protected:
        std::deque<Job> m_task_queue;          // 待执行任务队列 (FIFO)
        mutable std::mutex m_task_mutex;        // 保护队列
        std::atomic<size_t> m_task_count{0};    // 原子计数，供外部无锁查询

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

        // ---------------- 加入命令 ----------------

        // 加入任务（拷贝版）：入队后唤醒线程执行，执行完自动弹出
        void PushTask(const Job &task)
        {
            if (!IsValid(task)) return;
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                m_task_queue.push_back(task);
                m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
            }
            WakeUp();   // 注意：在 m_task_mutex 释放后再唤醒，避免与 WaitIfIdle 死锁
        }

        // 加入任务（移动版）
        void PushTask(Job &&task)
        {
            if (!IsValid(task)) return;
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                m_task_queue.push_back(std::move(task));
                m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
            }
            WakeUp();
        }

        // 插队：加入队首，下一个被执行（可用于紧急/高优先级命令）
        void PushTaskFront(Job &&task)
        {
            if (!IsValid(task)) return;
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                m_task_queue.push_front(std::move(task));
                m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
            }
            WakeUp();
        }

        // 清空所有尚未执行的任务
        void ClearTasks()
        {
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.clear();
            m_task_count.store(0, std::memory_order_relaxed);
        }

        // ---------------- 状态查询 ----------------

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

        // 可选：等队列全部执行完（排空）后再 Stop
        void StopAfterDrain()
        {
            while (m_is_running.load(std::memory_order_relaxed) && GetTaskCount() > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            Stop();
        }

    protected:
        // 单个 Job 的派生重载点：
        //  默认优先调用 task.Execute()；若 Job 是可调用对象（如 std::function / lambda）则直接调用
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

        // 核心单步逻辑：取出队首 -> 立即弹出 -> 在锁外执行
        void ProcessWork() override
        {
            // 队列无任务时挂起线程，防止空转（Stop / PushTask 会唤醒）
            WaitIfIdle([this]() {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                return !m_task_queue.empty();
            });

            if (!m_is_running.load(std::memory_order_relaxed))
                return; // 被 Stop 唤醒，直接结束本轮，让线程退出

            Job task;
            bool has_task = false;

            // 1) 上锁：从队首取出任务并立刻弹出（执行后不再保留）
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                while (!m_task_queue.empty())
                {
                    task = std::move(m_task_queue.front());
                    m_task_queue.pop_front();                       // <-- 弹出
                    m_task_count.store(m_task_queue.size(), std::memory_order_relaxed);
                    if (IsValid(task)) { has_task = true; break; }
                    // 无效任务：直接丢弃，继续取下一个
                }
            }

            // 2) 解锁后执行：执行期间其他线程仍可继续 PushTask 入队
            if (has_task)
            {
                ExecuteTask(task);
            }
            else
            {
                std::this_thread::yield();
            }
        }
    };
}
