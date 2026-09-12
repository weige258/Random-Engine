#pragma once
#include <atomic>
#include <chrono>
#include <thread>
#include "Jobs/Job/BaseJob.hpp"
#include "Jobs/JobWorker/LoopJobWorker.hpp"
#include "Behaviors/BaseBehavior/IFixUpdateBehavior.hpp"

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Jobs::JobWorker
{
    
    using FixUpdateBehaviorJob = Core::Jobs::Job::BaseJob<
        &Behaviors::IFixUpdateBehavior::FixUpdate,
        Config::TimeType,
        ::RandomEngine::Systems::System&>;

    /**
     * @brief 固定时间步 Worker（纯执行器）
     *
     * 与 LogicBehaviorJobWorker 的核心差异：
     *  - dt 不是真实帧间隔，而是恒定的 m_fixed_dt
     *  - 不自行计时——真实时间由 Executor 侧 Stepper 统一计量并发布 m_step_target
     *  - Worker 只负责追赶目标步号，以恒定 fixed_dt 执行行为
     *
     * 步边界对齐：所有 Worker 共享同一条 m_step_target 线，
     * 同一编号的步在所有 Worker 上对应同一逻辑时刻，支持跨 Worker 状态依赖。
     */
    class FixUpdateJobWorker : public LoopJobWorker<FixUpdateBehaviorJob>
    {
    private:
        // ---- 跨线程配置（Executor 写 / Worker 读，必须原子） ----
        std::atomic<::RandomEngine::Systems::System *> m_system{nullptr};
        std::atomic<Config::TimeType> m_fixed_dt{Config::TimeType{1} / 60}; // 固定步长（秒），默认 60Hz

        // ---- 步数同步（Executor 发布 / Worker 追赶） ----
        std::atomic<int64_t> *m_step_target_ptr = nullptr; // 指向 Executor 的全局步号
        std::atomic<int64_t>  m_done_step{0};              // 本 Worker 已执行到第几步（Worker 写 / Executor 读）

    public:
        FixUpdateJobWorker() = default;
        explicit FixUpdateJobWorker(::RandomEngine::Systems::System &system)
            : m_system(&system) {}
        ~FixUpdateJobWorker() override = default;

        void SetSystem(::RandomEngine::Systems::System &system)
        {
            m_system.store(&system, std::memory_order_release);
        }

        void SetFixedTimestep(Config::TimeType fixed_dt)
        {
            m_fixed_dt.store(fixed_dt > Config::TimeType{0} ? fixed_dt : (Config::TimeType{1} / 60),
                             std::memory_order_relaxed);
        }

        void SetStepTarget(std::atomic<int64_t> *target_ptr)
        {
            m_step_target_ptr = target_ptr;
        }

        [[nodiscard]] int64_t DoneStep() const noexcept
        {
            return m_done_step.load(std::memory_order_relaxed);
        }

    protected:
        void OnStart() override
        {
            if (m_step_target_ptr)
                m_done_step.store(m_step_target_ptr->load(std::memory_order_acquire),
                                  std::memory_order_relaxed);
        }

        void ProcessWork() override
        {
            // 0. 任务集变更时刷新本地缓存（复用 LoopJobWorker 的 dirty-cache 机制）
            if (m_dirty.exchange(false, std::memory_order_acquire))
            {
                std::lock_guard<std::mutex> lock(m_task_mutex);
                m_local_cache = m_dedicated_tasks;
            }

            // 1. 读节拍器发布的全局步号与固定步长
            const int64_t target   = m_step_target_ptr
                                         ? m_step_target_ptr->load(std::memory_order_acquire)
                                         : m_done_step.load(std::memory_order_relaxed);
            const Config::TimeType fixed_dt = m_fixed_dt.load(std::memory_order_relaxed);

            // 2. 追赶至 target：空 Worker 直接跳步，有任务的 Worker 逐步执行
            if (m_local_cache.empty())
            {
                m_done_step.store(target, std::memory_order_relaxed);
            }
            else
            {
                int64_t done = m_done_step.load(std::memory_order_relaxed);
                while (done < target)
                {
                    for (auto &job : m_local_cache)
                    {
                        if (job)
                            ExecuteJob(job, fixed_dt);
                    }
                    ++done;
                }
                m_done_step.store(done, std::memory_order_relaxed);
            }

            // 3. 已追上：短暂退让，等待节拍器发布新步号
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }

        void ExecuteJob(FixUpdateBehaviorJob &job, Config::TimeType fixed_dt)
        {
            ::RandomEngine::Systems::System *sys = m_system.load(std::memory_order_acquire);
            if (sys && job)
                job.Execute(fixed_dt, *sys);
        }
    };
}