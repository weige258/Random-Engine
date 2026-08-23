#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace RandEngine::Core::Job
{
    template <typename Task>
    class AffinityJobWorker
    {
    private:
        struct WorkerThreadData
        {
            std::thread thread;
            std::vector<Task *> dedicated_tasks; // 本线程专属固定的 Task 集合
            std::mutex task_mutex;               // 动态添加 Task 时的安全锁
            std::atomic<size_t> task_count{0};
        };

        std::vector<std::unique_ptr<WorkerThreadData>> m_workers;
        std::atomic<bool> m_is_running{false};
        std::atomic<size_t> m_rr_index{0};             // Round-Robin 分配索引
        std::atomic<bool> m_enable_auto_balance{true}; // 自动负载均衡

    public:
        AffinityJobWorker() = default;
        virtual ~AffinityJobWorker() { Stop(); }

        AffinityJobWorker(const AffinityJobWorker &) = delete;
        AffinityJobWorker &operator=(const AffinityJobWorker &) = delete;

        // 绑定 Task 到指定线程（target_thread_idx < 0 则按 Round-Robin 均摊绑定）
        bool PushTask(Task *task, int target_thread_idx = -1)
        {
            if (!task || m_workers.empty())
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

        // 在所有 Worker 线程中搜索并删除指定的 Task
        bool RemoveTask(Task *task)
        {
            if (!task || m_workers.empty())
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

        // 动态设置/扩缩工作线程数量（运行期设置可无损迁移现有任务）
        void SetThreadCount(size_t thread_count)
        {
            size_t count = (thread_count < 1) ? 1 : thread_count;
            bool was_running = m_is_running.load(std::memory_order_relaxed);

            std::vector<Task *> orphan_tasks;

            // 若在运行期调整，先停止现有线程并回收存量任务
            if (was_running)
            {
                m_is_running.store(false, std::memory_order_release);
                for (auto &w : m_workers)
                {
                    if (w && w->thread.joinable())
                    {
                        w->thread.join();
                    }
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

            // 重新分配 Worker 实例
            m_workers.clear();
            m_workers.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                m_workers.push_back(std::make_unique<WorkerThreadData>());
            }

            // 将原有任务均匀分发回新的 Worker 队列
            for (size_t i = 0; i < orphan_tasks.size(); ++i)
            {
                size_t target_idx = i % count;
                m_workers[target_idx]->dedicated_tasks.push_back(orphan_tasks[i]);
                m_workers[target_idx]->task_count.store(m_workers[target_idx]->dedicated_tasks.size(), std::memory_order_relaxed);
            }

            // 若原本处于运行状态，重新启动线程循环
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

            size_t count = (thread_count < 1) ? 1 : thread_count;

            m_workers.clear();
            m_workers.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                m_workers.push_back(std::make_unique<WorkerThreadData>());
            }

            m_is_running.store(true, std::memory_order_release);

            for (size_t i = 0; i < count; ++i)
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
                {
                    w->thread.join();
                }
            }
            m_workers.clear();
        }

    private:
        // 均衡负载函数
        void TryRebalanceTasks()
        {
            const size_t worker_cnt = m_workers.size();
            if (worker_cnt <= 1)
                return;

            constexpr size_t kMinGap = 2;         // 门槛 1：极差相差 >= 2 个 Task 才开始评估
            constexpr double kCvThreshold = 0.20; // 门槛 2：变异系数 (CV) 超过 20% 判定显著倾斜

            // 1. 无锁一趟采样：读取原子 count，消除采样阶段锁争用
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

            // 2. 剪枝 1：极差不满足直接退出
            if (max_val - min_val < kMinGap)
                return;

            // 3. 剪枝 2：计算方差与变异系数 (CV = std_dev / mean)
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
                    return; // 整体分布均匀，无需迁移
            }

            // 4. 执行迁移：仅在此处对两端的线程按升序加锁
            size_t first_idx = std::min(max_idx, min_idx);
            size_t second_idx = std::max(max_idx, min_idx);

            std::lock_guard<std::mutex> lock1(m_workers[first_idx]->task_mutex);
            std::lock_guard<std::mutex> lock2(m_workers[second_idx]->task_mutex);

            auto &src_tasks = m_workers[max_idx]->dedicated_tasks;
            auto &dst_tasks = m_workers[min_idx]->dedicated_tasks;

            if (src_tasks.size() > dst_tasks.size() + 1)
            {
                Task *moved_task = src_tasks.back();
                src_tasks.pop_back();
                dst_tasks.push_back(moved_task);

                // 迁移完成后同步更新无锁原子快照
                m_workers[max_idx]->task_count.store(src_tasks.size(), std::memory_order_relaxed);
                m_workers[min_idx]->task_count.store(dst_tasks.size(), std::memory_order_relaxed);
            }
        }

    protected:
        // 标准化基础接口：纯粹执行 Task，不携带任何框架默认参数
        virtual void ExecuteTask(Task *task) = 0;

        void WorkerLoop(size_t worker_index)
        {
            auto &worker = *m_workers[worker_index];
            std::vector<Task *> local_tasks_cache;

            while (m_is_running.load(std::memory_order_relaxed))
            {
                // 动态均衡负载函数
                if (worker_index == 0 && m_enable_auto_balance.load(std::memory_order_relaxed))
                {
                    TryRebalanceTasks();
                }

                // 1. 刷新同步本线程管辖的 Task 集合
                {
                    std::lock_guard<std::mutex> lock(worker.task_mutex);
                    local_tasks_cache = worker.dedicated_tasks;
                }

                // 2. 在本线程内死循环无抢夺执行属于自己的 Task 集合
                if (!local_tasks_cache.empty())
                {
                    for (Task *task : local_tasks_cache)
                    {
                        if (task)
                        {
                            ExecuteTask(task);
                        }
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