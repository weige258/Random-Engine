#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "Jobs/Job/BaseJob.hpp"
#include "Jobs/JobWorker/BaseLoopJobWorker.hpp"
#include "Jobs/Job/FixUpdateBehaviorJob.hpp"

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Jobs::JobWorker
{
    
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
    class FixUpdateJobWorker : public BaseLoopJobWorker<Jobs::Job::FixUpdateBehaviorJob>
    {
    private:
        // ---- 跨线程配置（Executor 写 / Worker 读，必须原子） ----
        std::atomic<::RandomEngine::Systems::System *> m_system{nullptr};
        std::atomic<Config::TimeType> m_fixed_dt{Config::TimeType{1} / 60}; // 固定步长（秒），默认 60Hz

        // ---- 步数同步（Executor 发布 / Worker 追赶） ----
        std::atomic<int64_t> *m_step_target_ptr = nullptr; // 指向 Executor 的全局步号
        std::atomic<int64_t>  m_done_step{0};              // 本 Worker 已执行到第几步（Worker 写 / Executor 读）

        // ---- Stepper 通知（Executor 注入） ----
        std::mutex              *m_step_mutex = nullptr;
        std::condition_variable *m_step_cv    = nullptr;

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

        void SetStepNotifier(std::mutex *m, std::condition_variable *cv)
        {
            m_step_mutex = m;
            m_step_cv    = cv;
        }

        [[nodiscard]] int64_t DoneStep() const noexcept
        {
            return m_done_step.load(std::memory_order_relaxed);
        }

        // 供 Executor 在 Pause/Resume 或步号线重建时把 Worker 的已执行步号与全局步号重新对齐
        void ResetDoneStep(int64_t step)
        {
            m_done_step.store(step, std::memory_order_relaxed);
        }

    protected:
        // 本类不睡在 m_cv 上（睡在 Stepper 的 m_step_cv），必须覆盖唤醒挂点，
        // 否则均衡器迁移进来的任务只能靠 50ms 超时才被看见
        void WakeUp() override
        {
            BaseLoopJobWorker::WakeUp();
            if (m_step_cv && m_step_mutex)
            {
                std::lock_guard<std::mutex> lock(*m_step_mutex);
                m_step_cv->notify_all();
            }
        }

        void OnStart() override
        {
            if (m_step_target_ptr)
                m_done_step.store(m_step_target_ptr->load(std::memory_order_acquire),
                                  std::memory_order_relaxed);
        }

        void ProcessWork() override
        {
            // 0. 任务集变更时刷新本地缓存（复用 BaseLoopJobWorker 的 dirty-cache 机制）
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
                // 不变式：Worker 不允许跑在全局步号线前面。
                // 步号线回退（Pause/Resume 归零）时 done 跟随回退，而不是空等 target 追上来。
                if (done > target) done = target;
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

            // 3. 已追上：等待 Stepper 发布新步号（notify_all 即时唤醒，超时仅防 Stepper 先挂死）
            // 谓词必须包含 m_dirty：迁移进来的任务靠 MarkDirty → WakeUp 提前返回，
            // 否则 notify 只是让谓词重判一次（不满足），仍要睡满 50ms 超时
            if (m_step_cv && m_step_mutex)
            {
                std::unique_lock<std::mutex> lk(*m_step_mutex);
                m_step_cv->wait_for(lk, std::chrono::milliseconds(50), [&]
                {
                    return !m_is_running.load(std::memory_order_relaxed)
                        || m_is_paused.load(std::memory_order_relaxed)
                        || m_dirty.load(std::memory_order_relaxed)
                        || (m_step_target_ptr
                            && m_step_target_ptr->load(std::memory_order_acquire)
                                > m_done_step.load(std::memory_order_relaxed));
                });
            }
            else
            {
                // 未注入 notifier 时不能空转，否则整条线程 100% 忙等打满一个核
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        void ExecuteJob(Jobs::Job::FixUpdateBehaviorJob &job, Config::TimeType fixed_dt)
        {
            ::RandomEngine::Systems::System *sys = m_system.load(std::memory_order_acquire);
            if (sys && job)
                job.Execute(fixed_dt, *sys);
        }
    };
}