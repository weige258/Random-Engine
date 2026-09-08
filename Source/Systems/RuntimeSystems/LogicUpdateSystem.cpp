#include "LogicUpdateSystem.hpp"
#include "Systems/System.hpp"
#include "Core/Jobs/Job/BaseJob.hpp"

namespace RandomEngine::Systems::RuntimeSystems
{

    void LogicUpdateSystem::ApplyBehaviorChanges(
        const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>>& added,
        const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>>& deleted)
    {
        for (const auto& task : deleted)
        {
            if (task)
            {
                logic_update_job_executor.RemoveJob(Core::Jobs::Job::BaseJob<&Core::Behaviors::ILogicUpdateBehavior::LogicUpdate,float,Systems::System&>(task));
            }
        }

        for (const auto& task : added)
        {
            if (task)
            {
                logic_update_job_executor.PushJob(Core::Jobs::Job::BaseJob<&Core::Behaviors::ILogicUpdateBehavior::LogicUpdate,float,Systems::System&>(task));
            }
        }
    }

    void LogicUpdateSystem::Init(System &system)
    {
        uint32_t require_thread_count = (system.device_system.cpu_system.GetCPUInfo().logical_processor_count / 4 );

        logic_update_job_executor.SetSystem(system);
        logic_update_job_executor.Start(require_thread_count);
    }

    void LogicUpdateSystem::Run(System &system)
    {
        auto [added, deleted] = system.resource_system.behavior_system
            .FetchBehaviorChangesToRuntimeSystem<Core::Behaviors::ILogicUpdateBehavior>();

        if (!added.empty() || !deleted.empty())
        {
            ApplyBehaviorChanges(added, deleted);
        }
    }

    void LogicUpdateSystem::Destroy()
    {
        logic_update_job_executor.Stop();
    }
}