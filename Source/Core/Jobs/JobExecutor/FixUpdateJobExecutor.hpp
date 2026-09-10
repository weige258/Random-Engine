#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "Memory/ObserverPtr.hpp"
#include "Jobs/JobWorker/FixUpdateJobWorker.hpp"
#include "Behaviors/BaseBehavior/IFixUpdateBehavior.hpp"
#include "Time/Timer.hpp"

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Jobs::JobExecutor
{
    /**
     * @brief 反馈伸缩决策机（P + I + D + AIMD 收缩）
     *
     * 输入原始 max_lag（步积压深度），输出行动指令：
     *   >0 → 扩 |N| 个 Worker
     *   <0 → 缩 |N| 个 Worker
     *   =0 → 不动
     *
     * P 项（误差分级）：lag<2→毛刺不动, lag 2~5→+1, lag>5→+2
     * D 项（排水检测）：lag 逐窗下降 ≥20% → 系统在回血，抑制扩
     * I 项（积分）：连续 3 次高压才扩，冷却 8s
     * 缩：AIMD——lag==0 持续 idle_threshold 秒 → -1（起始 10s，反弹翻倍至 60s）
     *     裁后 5s 内 lag 重现 → +1 撤裁 + 门槛翻倍
     */
    struct ScaleController
    {
        int   consecutive_high = 0;
        int   consecutive_low  = 0;
        std::chrono::steady_clock::time_point last_action_time{std::chrono::steady_clock::now()};
        std::chrono::steady_clock::time_point last_shrink_time;
        std::chrono::steady_clock::time_point idle_since;
        bool  shrink_pending = false;
        int   last_shrink_amount = 0;

        // ---- D 项：排水检测 ----
        int64_t prev_max_lag = 0;

        static constexpr int    kConsecutiveThreshold = 3;
        static constexpr int    kLagDepth             = 2;
        static constexpr int    kExpandCooldownSec    = 8;
        static constexpr int    kShrinkCooldownSec    = 10;
        static constexpr double kDrainRatio           = 0.20;

        void Reset()
        {
            consecutive_high   = 0;
            consecutive_low    = 0;
            last_action_time   = std::chrono::steady_clock::now();
            last_shrink_time   = {};
            idle_since         = {};
            shrink_pending     = false;
            last_shrink_amount = 0;
            prev_max_lag       = 0;
        }

        int Feed(int64_t max_lag, size_t cur, size_t upper, size_t lower)
        {
            const auto now = std::chrono::steady_clock::now();

            // ---- 卡顿保护：lag 异常大时可能是系统卡顿虚高，不视为真实压力 ----
            const bool maybe_stall = (max_lag > 1000);
            const bool pressure = (max_lag >= kLagDepth) && !maybe_stall;

            // ---- AIMD 反弹：裁后 5s 内 lag 重现 → 撤裁 + 冷却翻倍 ----
            if (shrink_pending && pressure)
            {
                if (now - last_shrink_time < std::chrono::seconds(5))
                {
                    last_action_time   = now;
                    consecutive_high   = 0;
                    consecutive_low    = 0;
                    idle_since         = {};
                    shrink_pending     = false;
                    prev_max_lag       = max_lag;
                    return last_shrink_amount;
                }
                shrink_pending = false;
            }

            if (pressure)
            {
                ++consecutive_high;
                consecutive_low = 0;
                idle_since      = {};
            }
            else
            {
                consecutive_high = 0;
                ++consecutive_low;
                if (idle_since == std::chrono::steady_clock::time_point{})
                    idle_since = now;
            }

            const auto elapsed = now - last_action_time;

            // ---- 扩容：P + I + D ----
            if (consecutive_high >= kConsecutiveThreshold &&
                cur < upper &&
                elapsed >= std::chrono::seconds(kExpandCooldownSec))
            {
                // ---- D 项：排水检测 ----
                const bool draining = (prev_max_lag > 0) &&
                    (static_cast<double>(max_lag) < static_cast<double>(prev_max_lag) * (1.0 - kDrainRatio));

                prev_max_lag = max_lag;

                if (draining)
                {
                    last_action_time = now;
                    consecutive_high = 0;
                    return 0;
                }

                // P 项：误差分级（卡顿虚高时保守 +1）
                int expand_n = (max_lag > 5 && !maybe_stall) ? 2 : 1;
                last_action_time = now;
                consecutive_high = 0;
                return std::min<int>(expand_n, static_cast<int>(upper - cur));
            }

            prev_max_lag = max_lag;

            // ---- 缩容 ----
            if (consecutive_low >= kConsecutiveThreshold && cur > lower)
            {
                // lag==0 快速缩容：系统已证明能轻松处理，4s 冷却即可
                if (max_lag == 0 && elapsed >= std::chrono::seconds(4))
                {
                    last_shrink_time   = now;
                    shrink_pending     = true;
                    last_shrink_amount = 1;
                    last_action_time   = now;
                    consecutive_low    = 0;
                    return -1;
                }

                // 空闲时长分级缩容：idle_since 不重置，持续累积
                const auto idle_sec = std::chrono::duration_cast<std::chrono::seconds>(
                    now - idle_since).count();

                // 冷却随空闲时长递减：越闲越敢动
                int cooldown = kShrinkCooldownSec;
                if (idle_sec >= 40)      cooldown = 4;
                else if (idle_sec >= 25) cooldown = 6;
                else if (idle_sec >= 15) cooldown = 8;

                if (elapsed >= std::chrono::seconds(cooldown))
                {
                    int shrink_n = 1;
                    if (idle_sec >= 40)      shrink_n = 4;
                    else if (idle_sec >= 25) shrink_n = 3;
                    else if (idle_sec >= 15) shrink_n = 2;

                    shrink_n = std::min<int>(shrink_n, static_cast<int>(cur - lower));

                    last_shrink_time   = now;
                    shrink_pending     = true;
                    last_shrink_amount = shrink_n;
                    last_action_time   = now;
                    consecutive_low    = 0;
                    return -shrink_n;
                }
            }

            return 0;
        }
    };

    /**
     * @brief 固定步长行为 Job 执行器
     *
     * 结构与 LogicBehaviorJobExecutor 对称：
     *  - 多 Worker +  RR 分发
     *  - 独立均衡线程（事件驱动 + 自适应退避 + 自动伸缩）
     *  - SetThreadCount 时增量逼近（不重建）
     *  - Stepper 线程统一计量真实时间、发布全局步号
     *
     * 额外职责：
     *   真实时间由 Stepper 统一读表、切步、发布 m_step_target；
     *   Worker 退化为纯执行器，只追目标步号，不自行计时。
     *
     * 自动伸缩：
     *   SetThreadCount(n) 设定基准（初始值 / 围绕中心 / 归位点）；
     *   均衡线程每 500ms 测量压力 → ScaleController 三态决策 → GrowOne/ShrinkOne 增量逼近。
     */
    class FixUpdateJobExecutor
    {
    private:
        // ---- 均衡策略常量（与 LogicBehaviorJobExecutor 一致） ----
        static constexpr size_t   kMinGap          = 2;
        static constexpr double   kCvThreshold     = 0.20;
        static constexpr size_t   kMaxBatchPerPass = 64;
        static constexpr uint32_t kMinTickMs       = 1;
        static constexpr uint32_t kMaxTickMs       = 16;
        static constexpr size_t   kKickInterval    = 16;

        // ---- 自动伸缩常量 ----
        static constexpr std::chrono::milliseconds kScaleInterval{500};

        std::vector<std::unique_ptr<Core::Jobs::JobWorker::FixUpdateJobWorker>> m_workers;
        mutable std::shared_mutex m_workers_mutex;

        ::RandomEngine::Systems::System *m_system = nullptr;

        // ---- 固定步长全局配置（Stepper 读 / 用户线程写，原子） ----
        std::atomic<float> m_fixed_dt{1.0f / 60.0f};
        std::atomic<int>   m_max_steps{5};

        // ---- Stepper：统一计量真实时间，发布全局步号 ----
        Core::Time::Timer     m_stepper_timer;
        std::atomic<int64_t>  m_step_target{0};
        std::thread           m_stepper_thread;
        std::atomic<bool>     m_stepper_stop{false};

        std::atomic<bool>   m_is_running{false};
        std::atomic<bool>   m_auto_balance{true};
        std::atomic<size_t> m_rr_index{0};

        // ---- 自动伸缩：人工基准 + 决策机 ----
        std::atomic<size_t> m_baseline{1};
        ScaleController     m_scaler;
        std::atomic<bool>   m_auto_scale{true};
        std::chrono::steady_clock::time_point m_last_scale_check;

        std::atomic<float>   m_cpu_ema{0.0f};
        std::atomic<int64_t> m_cpu_sample_ns{0};
        std::atomic<size_t>  m_hw_logical{0};
        std::atomic<size_t>  m_hw_physical{0};
        int                  m_saturate_count = 0;

        // ---- 控制面：均衡线程 ----
        std::thread             m_balance_thread;
        std::mutex              m_balance_mutex;
        std::condition_variable m_balance_cv;
        std::atomic<bool>       m_balance_request{false};
        std::atomic<bool>       m_balancer_stop{false};

    public:
        FixUpdateJobExecutor() = default;
        
        explicit FixUpdateJobExecutor(::RandomEngine::Systems::System &system , float fixed_dt = 1.0f / 60.0f, int max_steps = 5)
         {
            m_system = &system;
            SetFixedTimestep(fixed_dt);
            SetMaxSteps(max_steps);
         }

        ~FixUpdateJobExecutor() { Stop(); }

        FixUpdateJobExecutor(const FixUpdateJobExecutor &) = delete;
        FixUpdateJobExecutor &operator=(const FixUpdateJobExecutor &) = delete;

        void SetSystem(::RandomEngine::Systems::System &system)
        {
            m_system = &system;
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->SetSystem(system);
        }

        // ---------------- 固定步长配置（下发到所有 Worker） ----------------

        void SetFixedTimestep(float fixed_dt)
        {
            const float new_dt = fixed_dt > 0.0f ? fixed_dt : (1.0f / 60.0f);
            m_fixed_dt.store(new_dt, std::memory_order_release);
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->SetFixedTimestep(new_dt);
        }

        void SetMaxSteps(int max_steps)
        {
            m_max_steps.store(max_steps > 0 ? max_steps : 1, std::memory_order_release);
        }

        // ---------------- 生命周期 ----------------

        void Start(size_t thread_count = 1)
        {
            if (m_is_running.exchange(true, std::memory_order_acq_rel))
                return;

            const size_t n = std::max<size_t>(thread_count, 1);
            m_baseline.store(n, std::memory_order_relaxed);
            m_scaler.Reset();
            m_last_scale_check = std::chrono::steady_clock::now();

            {
                std::unique_lock<std::shared_mutex> lock(m_workers_mutex);
                BuildWorkers(n);
                for (auto &w : m_workers)
                    w->Start();
            }
            StartStepper();
            StartBalancer();
        }

        void Stop()
        {
            if (!m_is_running.exchange(false, std::memory_order_acq_rel))
                return;

            StopStepper();
            StopBalancer();

            std::unique_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->Stop();
            m_workers.clear();
        }

        void Pause()
        {
            m_stepper_timer.Stop();
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->Pause();
        }

        void Resume()
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
                if (w) w->Resume();
            m_stepper_timer.Start();
        }

        [[nodiscard]] bool IsRunning() const { return m_is_running.load(std::memory_order_relaxed); }

        [[nodiscard]] bool IsPaused() const
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (const auto &w : m_workers)
                if (w && !w->IsPaused()) return false;
            return !m_workers.empty();
        }

        // ---------------- 任务管理 ----------------

        bool PushJob(const Core::Jobs::JobWorker::FixUpdateBehaviorJob &job, int target_thread_idx = -1)
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            if (m_workers.empty()) return false;

            Core::Jobs::JobWorker::FixUpdateJobWorker *target = nullptr;

            if (target_thread_idx >= 0 &&
                static_cast<size_t>(target_thread_idx) < m_workers.size())
            {
                target = m_workers[static_cast<size_t>(target_thread_idx)].get();
            }
            else
            {
                const size_t idx = m_rr_index.fetch_add(1, std::memory_order_relaxed);
                target = m_workers[idx % m_workers.size()].get();
                if (idx % kKickInterval == 0)
                    KickBalancer();
            }

            target->PushTask(job);
            return true;
        }

        bool RemoveJob(const Core::Jobs::JobWorker::FixUpdateBehaviorJob &job)
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            for (auto &w : m_workers)
            {
                if (w && w->RemoveTask(job))
                {
                    KickBalancer();
                    return true;
                }
            }
            return false;
        }

        void SetAutoBalance(bool enable)
        {
            m_auto_balance.store(enable, std::memory_order_relaxed);
            if (enable) KickBalancer();
        }

        void SetAutoScale(bool enable)
        {
            m_auto_scale.store(enable, std::memory_order_relaxed);
        }

        struct ThreadBounds
        {
            size_t lower;
            size_t upper;
        };

        void UpdateCpuPressure(float cpu_usage_0_to_1, size_t hw_logical, size_t hw_physical)
        {
            constexpr float kAlpha = 0.2f;
            const float prev = m_cpu_ema.load(std::memory_order_relaxed);
            const float next = (m_cpu_sample_ns.load(std::memory_order_relaxed) == 0)
                ? cpu_usage_0_to_1
                : kAlpha * cpu_usage_0_to_1 + (1.0f - kAlpha) * prev;
            m_cpu_ema.store(next, std::memory_order_relaxed);
            m_cpu_sample_ns.store(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count(),
                std::memory_order_relaxed);

            if (m_hw_logical.load(std::memory_order_relaxed) == 0)
            {
                m_hw_logical.store(hw_logical, std::memory_order_relaxed);
                m_hw_physical.store(hw_physical, std::memory_order_relaxed);
            }
        }

        [[nodiscard]] ThreadBounds ComputeBounds()
        {
            return ComputeBoundsImpl(
                m_hw_logical.load(std::memory_order_relaxed),
                m_hw_physical.load(std::memory_order_relaxed),
                0);
        }

        [[nodiscard]] ThreadBounds ComputeBoundsImpl(size_t hw_logical, size_t hw_physical, int64_t max_lag) const
        {
            if (hw_logical == 0) hw_logical = std::thread::hardware_concurrency();
            if (hw_logical == 0) hw_logical = 4;
            if (hw_physical == 0 || hw_physical > hw_logical)
                hw_physical = (hw_logical > 1) ? hw_logical / 2 : 1;

            const size_t lower = m_baseline.load(std::memory_order_relaxed);

            constexpr size_t kSmtAllowance = 2;
            const size_t hw_cap = std::min(hw_logical, hw_physical + kSmtAllowance);
            const double headroom = (hw_cap > lower) ? static_cast<double>(hw_cap - lower) : 0.0;

            // CPU 使用率决定预算衰减：高 CPU → 少扩，低 CPU → 多扩
            // 有积压时旁路 decay——CPU 高恰恰是因为 Worker 在诚实干活
            const float cpu = m_cpu_ema.load(std::memory_order_relaxed);
            double decay = 1.0;
            if (max_lag == 0)
            {
                if (cpu >= 0.85f)       decay = 0.0;
                else if (cpu > 0.60f)   decay = 1.0 - static_cast<double>(cpu - 0.60f) / 0.25;
            }

            const size_t budget = static_cast<size_t>(headroom * decay);

            return {lower, lower + budget};
        }

        // ---------------- 运行时调整线程数（增量逼近，不停机） ----------------

        void SetThreadCount(size_t thread_count)
        {
            const size_t n = std::max<size_t>(thread_count, 1);
            m_baseline.store(n, std::memory_order_relaxed);
            m_scaler.Reset();

            if (!m_is_running.load(std::memory_order_relaxed))
                return;

            // 增量逼近：先扩后缩，每次只迁移最忙 Worker 的任务
            while (m_workers.size() < n)
                GrowOne();
            while (m_workers.size() > n)
                ShrinkOne();
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
                auto worker = std::make_unique<Core::Jobs::JobWorker::FixUpdateJobWorker>();
                if (m_system) worker->SetSystem(*m_system);
                worker->SetFixedTimestep(m_fixed_dt);
                worker->SetStepTarget(&m_step_target);
                m_workers.push_back(std::move(worker));
            }
        }

        // ================= 自动伸缩：增量扩缩 =================

        void GrowOne()
        {
            std::unique_lock<std::shared_mutex> lock(m_workers_mutex);

            auto worker = std::make_unique<Core::Jobs::JobWorker::FixUpdateJobWorker>();
            if (m_system) worker->SetSystem(*m_system);
            worker->SetFixedTimestep(m_fixed_dt.load(std::memory_order_relaxed));
            worker->SetStepTarget(&m_step_target);

            // 从最忙 Worker 迁移一半任务到新 Worker
            if (!m_workers.empty())
            {
                size_t max_idx = 0;
                size_t max_val = 0;
                for (size_t i = 0; i < m_workers.size(); ++i)
                {
                    const size_t c = m_workers[i]->GetTaskCount();
                    if (c > max_val) { max_val = c; max_idx = i; }
                }

                if (max_val > 1)
                {
                    auto lock_src = m_workers[max_idx]->LockQueue();
                    auto &src = m_workers[max_idx]->GetRawTasks();
                    auto &dst = worker->GetRawTasks();

                    const size_t move_n = src.size() / 2;
                    const auto first = src.end() - static_cast<std::ptrdiff_t>(move_n);
                    dst.insert(dst.end(),
                               std::make_move_iterator(first),
                               std::make_move_iterator(src.end()));
                    src.erase(first, src.end());

                    m_workers[max_idx]->UpdateTaskCount();
                    worker->UpdateTaskCount();
                    m_workers[max_idx]->MarkDirty();
                }
            }

            worker->Start();
            m_workers.push_back(std::move(worker));
        }

        void ShrinkOne()
        {
            std::unique_lock<std::shared_mutex> lock(m_workers_mutex);
            if (m_workers.size() <= 1) return;

            // 取末尾 Worker，先停后收
            auto &victim = m_workers.back();
            victim->Stop();

            // 收集孤儿任务，RR 分发到剩余 Worker
            std::vector<Core::Jobs::JobWorker::FixUpdateBehaviorJob> orphans;
            {
                auto queue_lock = victim->LockQueue();
                for (auto &t : victim->GetRawTasks())
                    if (t) orphans.push_back(std::move(t));
            }

            m_workers.pop_back();

            for (size_t i = 0; i < orphans.size(); ++i)
            {
                const size_t idx = m_rr_index.fetch_add(1, std::memory_order_relaxed);
                m_workers[idx % m_workers.size()]->PushTask(orphans[i]);
            }
        }

        int64_t MeasureMaxLag()
        {
            std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
            const int64_t target = m_step_target.load(std::memory_order_relaxed);

            int64_t max_lag = 0;

            for (auto &w : m_workers)
            {
                if (!w) continue;
                max_lag = std::max(max_lag, target - w->DoneStep());
            }

            // 返回原始步积压深度，由 ScaleController::Feed 做深度门槛 + P 分级 + D 排水
            return max_lag;
        }

        void StartStepper()
        {
            m_stepper_stop.store(false, std::memory_order_release);
            m_stepper_timer.Start();
            m_step_target.store(0, std::memory_order_release);
            m_stepper_thread = std::thread([this]() { StepperLoop(); });
        }

        void StopStepper()
        {
            m_stepper_stop.store(true, std::memory_order_release);
            if (m_stepper_thread.joinable())
                m_stepper_thread.join();
        }

        void StepperLoop()
        {
            float accumulator = 0.0f;

            while (!m_stepper_stop.load(std::memory_order_relaxed))
            {
                const float real_dt   = m_stepper_timer.GetDeltaTime();
                const float fixed_dt  = m_fixed_dt.load(std::memory_order_acquire);
                const int   max_steps = m_max_steps.load(std::memory_order_acquire);

                accumulator += real_dt;

                const float max_accum = static_cast<float>(max_steps) * fixed_dt;
                if (accumulator > max_accum)
                    accumulator = max_accum;

                while (accumulator >= fixed_dt)
                {
                    m_step_target.fetch_add(1, std::memory_order_release);
                    accumulator -= fixed_dt;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

                // ---- 自动伸缩：每 500ms 测量步积压 → P+I+D 决策 ----
                const auto now = std::chrono::steady_clock::now();
                if (now - m_last_scale_check >= kScaleInterval)
                {
                    m_last_scale_check = now;
                    if (m_auto_scale.load(std::memory_order_relaxed))
                    {
                        const int64_t max_lag = MeasureMaxLag();
                        const ThreadBounds tb = ComputeBoundsImpl(
                            m_hw_logical.load(std::memory_order_relaxed),
                            m_hw_physical.load(std::memory_order_relaxed),
                            max_lag);
                        const size_t cur = m_workers.size();

                        if (cur < tb.lower)
                        {
                            for (size_t i = 0; i < tb.lower - cur; ++i) GrowOne();
                        }
                        else if (cur > tb.upper && max_lag == 0)
                        {
                            // 硬约束：worker 数必须在 [lower, upper] 范围内
                            for (size_t i = 0; i < cur - tb.upper; ++i) ShrinkOne();
                        }
                        else
                        {
                            const int act = m_scaler.Feed(max_lag, cur, tb.upper, tb.lower);

                            if (act > 0)
                            {
                                for (int i = 0; i < act; ++i) GrowOne();
                            }
                            else if (act < 0)
                            {
                                for (int i = 0; i < -act; ++i) ShrinkOne();
                            }

                            if (max_lag > 0 && cur >= tb.upper)
                            {
                                if (++m_saturate_count >= 6)
                                {
                                    m_saturate_count = 0;
                                }
                            }
                        }
                    }
                }

                bool migrated = false;
                {
                    std::shared_lock<std::shared_mutex> lock(m_workers_mutex);
                    if (m_workers.size() > 1)
                    {
                        std::vector<Core::Jobs::JobWorker::FixUpdateJobWorker *> workers;
                        workers.reserve(m_workers.size());
                        for (auto &w : m_workers)
                            workers.push_back(w.get());
                        migrated = RebalancePass(workers);
                    }
                }

                tick_ms = migrated ? kMinTickMs
                                   : std::min<uint32_t>(tick_ms * 2, kMaxTickMs);
            }
        }

        bool RebalancePass(const std::vector<Core::Jobs::JobWorker::FixUpdateJobWorker *> &workers)
        {
            const size_t n = workers.size();

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

            const size_t lo = std::min(max_idx, min_idx);
            const size_t hi = std::max(max_idx, min_idx);
            auto lock_lo = workers[lo]->LockQueue();
            auto lock_hi = workers[hi]->LockQueue();

            auto &src = workers[max_idx]->GetRawTasks();
            auto &dst = workers[min_idx]->GetRawTasks();

            if (src.size() <= dst.size() + 1)
                return false;

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

            // 关键：迁移后两边都必须置 dirty，否则：
            //   源 Worker: m_local_cache 仍持有已迁走的任务 → 幽灵执行
            //   目标 Worker: m_local_cache 永不刷新 → 不执行新任务；
            //               直到某次 Push 触发 dirty 时 → 双重执行
            workers[max_idx]->MarkDirty();
            workers[min_idx]->MarkDirty();
            return true;
        }
    };
}