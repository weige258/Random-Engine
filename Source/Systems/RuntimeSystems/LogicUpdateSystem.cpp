#include "LogicUpdateSystem.hpp"
#include "Systems/System.hpp"

namespace RandEngine::Systems::RuntimeSystems
{

    void LogicUpdateSystem::ApplyBehaviorChanges(
        const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>>& added,
        const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>>& deleted)
    {

        for (const auto& task : deleted)
        {
            if (task)
            {
                logic_behavior_job_worker.RemoveTask(task);
            }
        }

        for (const auto& task : added)
        {
            if (task)
            {
                logic_behavior_job_worker.PushTask(task);
            }
        }
    }

    void LogicUpdateSystem::Init(System &system)
    {
        logic_behavior_job_worker.SetSystem(system);

        uint32_t require_thread_count = (system.device_system.cpu_system.GetCPUInfo().logical_processor_count / 4);
        logic_behavior_job_worker.Start(require_thread_count);
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
    }
}