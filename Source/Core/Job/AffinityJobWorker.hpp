#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace RandEngine::Core::Job
{
    template <typename Task>
    class AffinityJobWorker
    {
    private:
        struct WorkerThreadData
        {
            std::thread thread;
            std::vector<Task> dedicated_tasks;
            std::mutex task_mutex;
            std::atomic<size_t> task_count{0};
        };

        std::vector<std::unique_ptr<WorkerThreadData>> m_workers;
        std::atomic<bool> m_is_running{false};
        std::atomic<bool> m_is_paused{false};
        std::atomic<size_t> m_rr_index{0};
        std::atomic<bool> m_enable_auto_balance{true};

        static bool IsValid(const Task &task)
        {
            if constexpr (std::is_pointer_v<Task>)
                return task != nullptr;
            else
                return static_cast<bool>(task);
        }

    public:
        AffinityJobWorker() = default;
        virtual ~AffinityJobWorker() { Stop(); }

        AffinityJobWorker(const AffinityJobWorker &) = delete;
        AffinityJobWorker &operator=(const AffinityJobWorker &) = delete;

        bool PushTask(const Task &task, int target_thread_idx = -1)
        {
            if (!IsValid(task) || m_workers.empty())
                return false;

            size_t worker_idx = 0;
            if (target_thread_idx >= 0 && static_cast<size_t>(target_thread_idx) < m_workers.size())
            {
                worker_idx = static_cast<size_t>(target_thread_idx);
            }
            else
            {
                worker_idx = m_rr_index.fetch_add(1, std::memory_order_relaxed) % m_workers.size();
            }

            auto &worker = *m_workers[worker_idx];
            {
                std::lock_guard<std::mutex> lock(worker.task_mutex);
                worker.dedicated_tasks.push_back(task);
                worker.task_count.store(worker.dedicated_tasks.size(), std::memory_order_relaxed);
            }
            return true;
        }

        bool RemoveTask(const Task &task)
        {
            if (!IsValid(task) || m_workers.empty())
                return false;

            for (auto &worker_ptr : m_workers)
            {
                if (!worker_ptr)
                    continue;

                std::lock_guard<std::mutex> lock(worker_ptr->task_mutex);
                auto &tasks = worker_ptr->dedicated_tasks;
                auto it = std::remove(tasks.begin(), tasks.end(), task);
                if (it != tasks.end())
                {
                    tasks.erase(it, tasks.end());
                    worker_ptr->task_count.store(tasks.size(), std::memory_order_relaxed);
                    return true;
                }
            }
            return false;
        }

        void SetThreadCount(size_t thread_count)
        {
            size_t count = (thread_count < 1) ? 1 : thread_count;
            bool was_running = m_is_running.load(std::memory_order_relaxed);

            std::vector<Task> orphan_tasks;

            if (was_running)
            {
                m_is_running.store(false, std::memory_order_release);
                for (auto &w : m_workers)
                {
                    if (w && w->thread.joinable())
                        w->thread.join();
                }

                for (auto &w : m_workers)
                {
                    if (w)
                    {
                        std::lock_guard<std::mutex> lock(w->task_mutex);
                        orphan_tasks.insert(orphan_tasks.end(), w->dedicated_tasks.begin(), w->dedicated_tasks.end());
                    }
                }
            }

            m_workers.clear();
            m_workers.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                m_workers.push_back(std::make_unique<WorkerThreadData>());
            }

            for (size_t i = 0; i < orphan_tasks.size(); ++i)
            {
                size_t target_idx = i % count;
                m_workers[target_idx]->dedicated_tasks.push_back(orphan_tasks[i]);
                m_workers[target_idx]->task_count.store(m_workers[target_idx]->dedicated_tasks.size(), std::memory_order_relaxed);
            }

            if (was_running)
            {
                m_is_running.store(true, std::memory_order_release);
                for (size_t i = 0; i < count; ++i)
                {
                    m_workers[i]->thread = std::thread([this, i]()
                                                       { WorkerLoop(i); });
                }
            }
        }

        void SetAutoBalance(bool enable)
        {
            m_enable_auto_balance.store(enable, std::memory_order_relaxed);
        }

        void Start(size_t thread_count)
        {
            if (m_is_running.load(std::memory_order_relaxed))
                return;

            SetThreadCount(thread_count);

            m_is_running.store(true, std::memory_order_release);

            for (size_t i = 0; i < m_workers.size(); ++i)
            {
                m_workers[i]->thread = std::thread([this, i]()
                                                   { WorkerLoop(i); });
            }
        }

        void Stop()
        {
            if (!m_is_running.exchange(false, std::memory_order_acq_rel))
                return;

            for (auto &w : m_workers)
            {
                if (w && w->thread.joinable())
                    w->thread.join();
            }
            m_workers.clear();
        }

        void Pause() { m_is_paused.store(true, std::memory_order_release); }
        void Resume() { m_is_paused.store(false, std::memory_order_release); }
        [[nodiscard]] bool IsPaused() const { return m_is_paused.load(std::memory_order_relaxed); }

    private:
        void TryRebalanceTasks()
        {
            const size_t worker_cnt = m_workers.size();
            if (worker_cnt <= 1)
                return;

            constexpr size_t kMinGap = 2;
            constexpr double kCvThreshold = 0.20;

            std::vector<size_t> counts(worker_cnt);
            size_t max_idx = 0, min_idx = 0;
            size_t max_val = 0, min_val = std::numeric_limits<size_t>::max();
            size_t sum_tasks = 0;

            for (size_t i = 0; i < worker_cnt; ++i)
            {
                size_t count = m_workers[i]->task_count.load(std::memory_order_relaxed);
                counts[i] = count;
                sum_tasks += count;

                if (count > max_val)
                {
                    max_val = count;
                    max_idx = i;
                }
                if (count < min_val)
                {
                    min_val = count;
                    min_idx = i;
                }
            }

            if (max_val - min_val < kMinGap)
                return;

            double mean = static_cast<double>(sum_tasks) / worker_cnt;
            if (mean > 0.0)
            {
                double variance_sum = 0.0;
                for (size_t count : counts)
                {
                    double diff = static_cast<double>(count) - mean;
                    variance_sum += diff * diff;
                }
                double std_dev = std::sqrt(variance_sum / worker_cnt);
                if ((std_dev / mean) < kCvThreshold)
                    return;
            }

            size_t first_idx = std::min(max_idx, min_idx);
            size_t second_idx = std::max(max_idx, min_idx);

            std::lock_guard<std::mutex> lock1(m_workers[first_idx]->task_mutex);
            std::lock_guard<std::mutex> lock2(m_workers[second_idx]->task_mutex);

            auto &src_tasks = m_workers[max_idx]->dedicated_tasks;
            auto &dst_tasks = m_workers[min_idx]->dedicated_tasks;

            if (src_tasks.size() > dst_tasks.size() + 1)
            {
                Task moved_task = src_tasks.back();
                src_tasks.pop_back();
                dst_tasks.push_back(moved_task);

                m_workers[max_idx]->task_count.store(src_tasks.size(), std::memory_order_relaxed);
                m_workers[min_idx]->task_count.store(dst_tasks.size(), std::memory_order_relaxed);
            }
        }

    protected:
        virtual void ExecuteTask(Task task) = 0;

        void WorkerLoop(size_t worker_index)
        {
            auto &worker = *m_workers[worker_index];
            std::vector<Task> local_tasks_cache;

            while (m_is_running.load(std::memory_order_relaxed))
            {
                
                if (m_is_paused.load(std::memory_order_relaxed))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                if (worker_index == 0 && m_enable_auto_balance.load(std::memory_order_relaxed))
                {
                    TryRebalanceTasks();
                }

                {
                    std::lock_guard<std::mutex> lock(worker.task_mutex);
                    auto &tasks = worker.dedicated_tasks;
                    local_tasks_cache.clear(); // 保持 vector 预留 Capacity，无堆内存分配

                    bool has_invalid = false;

                    // 1. 单趟遍历：提取有效任务，标记是否存在失效指针
                    for (const auto &task : tasks)
                    {
                        if (IsValid(task))
                        {
                            local_tasks_cache.push_back(task);
                        }
                        else
                        {
                            has_invalid = true;
                        }
                    }

                    // 2. 发现脏数据时，顺带回写同步并修正 task_count
                    if (has_invalid)
                    {
                        tasks = local_tasks_cache;
                        worker.task_count.store(tasks.size(), std::memory_order_relaxed);
                    }
                }

                // 3. 执行当前轮次的有效任务（100% 保证指针有效）
                if (!local_tasks_cache.empty())
                {
                    for (const auto &task : local_tasks_cache)
                    {
                        ExecuteTask(task);
                    }
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        }
    };
}