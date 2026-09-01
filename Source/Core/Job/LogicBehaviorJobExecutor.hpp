#pragma once
#include "Memory/ObserverPtr.hpp"
#include "AffinityJobExecutor.hpp"
#include "Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Time/Timer.hpp"
#include "vector"
#include "unordered_set"

namespace RandEngine::Core::Job
{

    struct LogicBehaviorJobExecutor : public AffinityJobExecutor<Memory::ObserverPtr<Behaviors::ILogicUpdateBehavior>>
    {
    private:
       RandEngine::Systems::System *m_system = nullptr;

    public:
        LogicBehaviorJobExecutor() = default;

        LogicBehaviorJobExecutor(RandEngine::Systems::System &system)
            : m_system(&system) {}
        ~LogicBehaviorJobExecutor() override = default;

        void SetSystem(RandEngine::Systems::System &system){
            m_system = &system;
        }

        void ExecuteTask(Memory::ObserverPtr<Behaviors::ILogicUpdateBehavior> task) override
        {
            if (!task || !m_system)
                return;

            // 1. 每个 Worker 线程各自持有一套独立的 Timer 和状态变量
            thread_local Core::Time::Timer tls_timer;
            thread_local float tls_cached_dt = 0.0f;
            thread_local const void *tls_first_task_anchor = nullptr;
            thread_local bool tls_initialized = false;
            thread_local size_t tls_task_count_since_anchor = 0;

            const void *current_task_ptr = task.Get();

            if (!tls_initialized)
            {
                // 线程启动后处理第一个任务时启动 Timer 并记录锚点
                tls_timer.Start();
                tls_initialized = true;
                tls_cached_dt = tls_timer.GetDeltaTime();
                tls_first_task_anchor = current_task_ptr;
                tls_task_count_since_anchor = 0;
            }
            else
            {
                // 2. 判断是否回到当前线程任务队列的开头（开启了新一轮循环）
                if (current_task_ptr == tls_first_task_anchor)
                {
                    // 为当前 Worker 线程更新本轮循环的 Delta Time
                    tls_cached_dt = tls_timer.GetDeltaTime();
                    tls_task_count_since_anchor = 0;
                }
                else
                {
                    tls_task_count_since_anchor++;

                    // 容错机制：若首个任务被动态移除，导致找不到 anchor，达到计数上限后自动重置 anchor
                    if (tls_task_count_since_anchor > 2048)
                    {
                        tls_first_task_anchor = current_task_ptr;
                        tls_cached_dt = tls_timer.GetDeltaTime();
                        tls_task_count_since_anchor = 0;
                    }
                }
            }

            // 3. 当前线程本轮循环内的所有 Behavior 使用完全相同的 tls_cached_dt
            task->LogicUpdate(tls_cached_dt, *m_system);
        }
    };
}