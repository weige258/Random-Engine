#pragma once
#include "Jobs/Job/BaseJob.hpp"
#include "Jobs/JobWorker/BaseLoopJobWorker.hpp"
#include "Jobs/Job/LogicUpdateBehaviorJob.hpp"
#include "Time/Timer.hpp"

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Jobs::JobWorker
{
    
  
    class LogicBehaviorJobWorker : public BaseLoopJobWorker<RandomEngine::Core::Jobs::Job::LogicUpdateBehaviorJob>
    {
    private:
        ::RandomEngine::Systems::System *m_system = nullptr;
        Core::Time::Timer m_timer;
        Config::TimeType m_dt = Config::TimeType{0};

    public:
        LogicBehaviorJobWorker() = default;

        explicit LogicBehaviorJobWorker(::RandomEngine::Systems::System &system)
            : m_system(&system) {}

        ~LogicBehaviorJobWorker() override = default;

        void SetSystem(::RandomEngine::Systems::System &system)
        {
            m_system = &system;
        }

    protected:
        // Worker 线程创建启动时触发一次
        void OnStart() override
        {
            m_timer.Start();
        }

        // 每轮任务循环开始前触发：在线程本地安全更新当前轮次的 dt，无锁且无需 thread_local
        void OnLoopStart() override
        {
            m_dt = m_timer.GetDeltaTime();

            // 空闲时 Worker 阻塞在 m_cv 上，唤醒后首帧 dt 会等于整个空闲时长；
            // 直接送入 velocity 积分会炸，按最小 15fps 夹取
            constexpr Config::TimeType kMaxDt = Config::TimeType{1} / 15;
            if (m_dt > kMaxDt) m_dt = kMaxDt;
        }

        // 逐 Task 执行点：将 Worker 持有的上下文参数（m_dt, *m_system）灌入 Job
        void ExecuteJob(RandomEngine::Core::Jobs::Job::LogicUpdateBehaviorJob &job) override
        {
            if (m_system && job)
            {
                job.Execute(m_dt, *m_system);
            }
        }
    };
}