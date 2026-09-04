#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "Memory/ObserverPtr.hpp"
#include "Jobs/JobWorker/LogicBehaviorJobWorker.hpp"
#include "Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"

namespace RandomEngine::Systems
{
    struct System;
}

namespace RandomEngine::Core::Jobs::JobExecutor
{
    class LogicBehaviorJobExecutor
    {
    private:
        // ---- 均衡策略常量 ----
        static constexpr size_t   kMinGap          = 2;    // max-min 小于此值不迁移
        static constexpr double   kCvThreshold     = 0.20; // 变异系数阈值，过滤临界抖动
        static constexpr size_t   kMaxBatchPerPass = 64;   // 单次迁移上限（限制锁持有时长）
        static constexpr uint32_t kMinTickMs       = 1;    // 均衡线程最小 tick
        static constexpr uint32_t kMaxTickMs       = 16;   // 空闲指数退避上限
        static constexpr size_t   kKickInterval    = 16;   // 每 N 次 RR Push 才唤醒一次均衡线程

        std::vector<std::unique_ptr<Core::Jobs::JobWorker::LogicBehaviorJobWorker>> m_workers;
        mutable std::shared_mutex m_workers_mutex; // shared: Push/Remove/均衡采样; unique: 重建

        ::RandomEngine::Systems::System *m_system = nullptr;

        std::atomic<bool>   m_is_running{false};
        std::atomic<bool>   m_auto_balance{true};
        std::atomic<size_t> m_rr_index{0};

        // ---- 控制面：独立均衡线程 ----
        std::thread             m_balance_thread;
        std::mutex              m_balance_mutex;
        std::condition_variable m_balance_cv;
        std::atomic<bool>       m_balance_request{false}; // 多次 Kick 自动合并为一次 Pass
        std::atomic<bool>       m_balancer_stop{false};

    public:
        LogicBehaviorJobExecutor() = default;
        explicit LogicBehaviorJobExecutor(::RandomEngine::Systems::System &system)
            : m_system(&system) {}
        ~LogicBehaviorJobExecutor() { Stop(); }

        LogicBehaviorJobExecutor(const LogicBehaviorJobExecutor &) = delete;
        LogicBehaviorJobExecutor &operator=(const LogicBehaviorJobExecutor &) = delete;

        void SetSystem(::RandomEngine::Systems::System &system)
        {
            m_system = &system;
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->SetSystem(system);
        }

        // ---------------- 生命周期 ----------------

        void Start(size_t thread_count = 1)
        {
            if (m_is_running.exchange(true, std::memory_order_acq_rel))
                return;

            {
                std::unique_lock<std::shared_mutex> lock(m_workers_mutex);
                BuildWorkers(thread_count);
                for (auto &w : m_workers)
                    w->Start(); // dt/Tick 逻辑全部在 Worker 内部（OnStart/OnLoopStart）
            }

            StartBalancer();
        }

        void Stop()
        {
            if (!m_is_running.exchange(false, std::memory_order_acq_rel))
                return;

            StopBalancer(); // 先停控制面，避免均衡 Pass 引用正在销毁的 Worker

            std::unique_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->Stop();
            m_workers.clear();
        }

        void Pause()
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->Pause(); // BaseJobWorker: cv 挂起，不烧 CPU
        }

        void Resume()
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->Resume();
        }

        [[nodiscard]] bool IsRunning() const { return m_is_running.load(std::memory_order_relaxed); }

        [[nodiscard]] bool IsPaused() const
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (const auto &w : m_workers)
                if (w && !w->IsPaused())
                    return false;
            return !m_workers.empty();
        }

        // ---------------- 任务管理 ----------------

        bool PushJob(const Core::Jobs::JobWorker::LogicBehaviorJob &job, int target_thread_idx = -1)
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            if (m_workers.empty())
                return false;

            Core::Jobs::JobWorker::LogicBehaviorJobWorker *target = nullptr;

            if (target_thread_idx >= 0 &&
                static_cast<size_t>(target_thread_idx) < m_workers.size())
            {
                target = m_workers[static_cast<size_t>(target_thread_idx)].get();
            }
            else
            {
                const size_t idx = m_rr_index.fetch_add(1, std::memory_order_relaxed);
                target = m_workers[idx % m_workers.size()].get();

                // 事件驱动均衡：每 kKickInterval 次 RR 推送踢一次均衡线程；
                // Push 风暴中多次 Kick 会被 request 原子标志合并成一次 Pass
                if (idx % kKickInterval == 0)
                    KickBalancer();
            }

            target->PushTask(job); // Worker 内部持队列锁入队 + WakeUp
            return true;
        }

        bool RemoveJob(const Core::Jobs::JobWorker::LogicBehaviorJob &job)
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
            {
                if (w && w->RemoveTask(job))
                {
                    KickBalancer(); // 移除同样会造成倾斜，通知控制面补偿
                    return true;
                }
            }
            return false;
        }

        void SetAutoBalance(bool enable)
        {
            m_auto_balance.store(enable, std::memory_order_relaxed);
            if (enable)
                KickBalancer();
        }

        // ---------------- 运行时调整线程数（迁移孤儿任务） ----------------

        void SetThreadCount(size_t thread_count)
        {
            const size_t count = thread_count < 1 ? 1 : thread_count;
            const bool was_running = m_is_running.load(std::memory_order_relaxed);

            // unique 锁：挡住所有 Push / 均衡 Pass，独占重建
            std::unique_lock<std::shared_mutex> lock(m_workers_mutex);

            std::vector<Core::Jobs::JobWorker::LogicBehaviorJob> orphans;

            if (was_running)
                for (auto &w : m_workers)
                    if (w) w->Stop();

            for (auto &w : m_workers) // 回收孤儿任务
            {
                if (!w) continue;
                auto queue_lock = w->LockQueue();
                for (auto &t : w->GetRawTasks())
                    if (t) orphans.push_back(std::move(t));
            }

            BuildWorkers(count);
            for (size_t i = 0; i < orphans.size(); ++i)
                m_workers[i % count]->PushTask(orphans[i]);

            if (was_running)
                for (auto &w : m_workers)
                    w->Start();
        }

        // ---------------- 状态查询 ----------------

        [[nodiscard]] size_t GetWorkerCount() const
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            return m_workers.size();
        }

        [[nodiscard]] size_t GetTaskCount(size_t worker_idx) const
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            return (worker_idx < m_workers.size() && m_workers[worker_idx])
                       ? m_workers[worker_idx]->GetTaskCount() : 0;
        }

        [[nodiscard]] size_t GetTotalTaskCount() const
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            size_t sum = 0;
            for (const auto &w : m_workers)
                if (w) sum += w->GetTaskCount();
            return sum;
        }

    private:
        void BuildWorkers(size_t count)
        {
            m_workers.clear();
            m_workers.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                auto worker = std::make_unique<Core::Jobs::JobWorker::LogicBehaviorJobWorker>();
                if (m_system)
                    worker->SetSystem(*m_system);
                m_workers.push_back(std::move(worker));
            }
        }

        // ================= 控制面：均衡线程 =================

        void StartBalancer()
        {
            m_balancer_stop.store(false, std::memory_order_release);
            m_balance_thread = std::thread([this]() { BalanceLoop(); });
        }

        void StopBalancer()
        {
            m_balancer_stop.store(true, std::memory_order_release);
            {
                std::lock_guard<std::mutex> lock(m_balance_mutex);
                m_balance_cv.notify_all();
            }
            if (m_balance_thread.joinable())
                m_balance_thread.join();
        }

        void KickBalancer()
        {
            m_balance_request.store(true, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(m_balance_mutex);
            m_balance_cv.notify_one();
        }

        void BalanceLoop()
        {
            uint32_t tick_ms = kMinTickMs;

            while (!m_balancer_stop.load(std::memory_order_relaxed) &&
                   m_is_running.load(std::memory_order_relaxed))
            {
                // 事件驱动 + 超时兜底：Push/Remove 立即唤醒；无事件时按 tick 退避
                {
                    std::unique_lock<std::mutex> lock(m_balance_mutex);
                    m_balance_cv.wait_for(lock, std::chrono::milliseconds(tick_ms), [this]
                    {
                        return m_balancer_stop.load(std::memory_order_relaxed) ||
                               m_balance_request.exchange(false, std::memory_order_relaxed);
                    });
                }

                if (m_balancer_stop.load(std::memory_order_relaxed) ||
                    !m_is_running.load(std::memory_order_relaxed))
                    break;

                if (!m_auto_balance.load(std::memory_order_relaxed))
                    continue;

                bool migrated = false;
                {
                    // shared 锁：与 Push/Remove 并行；与 SetThreadCount 的 unique 锁互斥
                    std::shared_lock<std::shared_mutex> lock(m_workers_mutex);

                    if (m_workers.size() > 1)
                    {
                        std::vector<Core::Jobs::JobWorker::LogicBehaviorJobWorker *> workers;
                        workers.reserve(m_workers.size());
                        for (auto &w : m_workers)
                            workers.push_back(w.get());

                        migrated = RebalancePass(workers);
                    }
                }

                // 自适应退避：发生过迁移保持高频；连续无事可做则 tick 指数增大
                tick_ms = migrated ? kMinTickMs
                                   : std::min<uint32_t>(tick_ms * 2, kMaxTickMs);
            }
        }

        bool RebalancePass(const std::vector<Core::Jobs::JobWorker::LogicBehaviorJobWorker *> &workers)
        {
            const size_t n = workers.size();

            // 1. 纯原子采样，无锁快速路径
            std::vector<size_t> counts(n);
            size_t sum = 0, max_val = 0;
            size_t min_val = std::numeric_limits<size_t>::max();
            size_t max_idx = 0, min_idx = 0;

            for (size_t i = 0; i < n; ++i)
            {
                const size_t c = workers[i]->GetTaskCount();
                counts[i] = c;
                sum += c;
                if (c > max_val) { max_val = c; max_idx = i; }
                if (c < min_val) { min_val = c; min_idx = i; }
            }

            if (max_val - min_val < kMinGap)
                return false;

            // 2. 变异系数过滤：接近均衡时不折腾
            const double mean = static_cast<double>(sum) / static_cast<double>(n);
            if (mean > 0.0)
            {
                double variance = 0.0;
                for (const size_t c : counts)
                {
                    const double d = static_cast<double>(c) - mean;
                    variance += d * d;
                }
                if (std::sqrt(variance / static_cast<double>(n)) / mean < kCvThreshold)
                    return false;
            }

            // 3. 固定锁序（小下标→大下标）；均衡线程是全系统唯一的多队列锁持有者，无死锁环
            const size_t lo = std::min(max_idx, min_idx);
            const size_t hi = std::max(max_idx, min_idx);
            auto lock_lo = workers[lo]->LockQueue();
            auto lock_hi = workers[hi]->LockQueue();

            auto &src = workers[max_idx]->GetRawTasks();
            auto &dst = workers[min_idx]->GetRawTasks();

            if (src.size() <= dst.size() + 1)
                return false;

            // 4. 批量迁移：一次搬走差距的一半（封顶），O(log gap) 轮收敛
            size_t move_n = std::min<size_t>((src.size() - dst.size()) / 2, kMaxBatchPerPass);
            if (move_n == 0)
                return false;

            const auto first = src.end() - static_cast<std::ptrdiff_t>(move_n);
            dst.insert(dst.end(),
                       std::make_move_iterator(first),
                       std::make_move_iterator(src.end()));
            src.erase(first, src.end());

            workers[max_idx]->UpdateTaskCount();
            workers[min_idx]->UpdateTaskCount();
            return true;
        }
    };
}
