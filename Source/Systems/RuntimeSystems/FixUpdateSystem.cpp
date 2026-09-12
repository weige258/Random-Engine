#include "FixUpdateSystem.hpp"
#include "System.hpp"

namespace RandomEngine::Systems::RuntimeSystems
{

    void FixUpdateSystem::ApplyBehaviorChanges(
        const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::IFixUpdateBehavior>> &added,
        const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::IFixUpdateBehavior>> &deleted)
    {
        for (const auto &task : deleted)
        {
            if (task)
            {
                fix_update_job_executor.RemoveJob(Core::Jobs::Job::BaseJob<&Core::Behaviors::IFixUpdateBehavior::FixUpdate, RandomEngine::Core::Config::TimeType, Systems::System &>(task));
            }
        }

        for (const auto &task : added)
        {
            if (task)
            {
                fix_update_job_executor.PushJob(Core::Jobs::Job::BaseJob<&Core::Behaviors::IFixUpdateBehavior::FixUpdate, RandomEngine::Core::Config::TimeType, Systems::System &>(task));
            }
        }
    }

    void FixUpdateSystem::Init(System &system)
    {

        fix_update_job_executor.SetSystem(system);
        fix_update_job_executor.SetFixedTimestep(fixed_delta_time);
        fix_update_job_executor.SetMaxSteps(5);
        fix_update_job_executor.Start(system.device_system.cpu_system.GetCPUInfo().logical_processor_count/6 );
    }

    void FixUpdateSystem::Run(System &system)
    {
        UpdateCpuPressure(system);

        auto [added, deleted] = system.resource_system.behavior_system
                                    .FetchBehaviorChangesToRuntimeSystem<Core::Behaviors::IFixUpdateBehavior>();

        if (!added.empty() || !deleted.empty())
        {
            ApplyBehaviorChanges(added, deleted);
        }
    }

    void FixUpdateSystem::Destroy()
    {
        fix_update_job_executor.Stop();
    }

    void FixUpdateSystem::UpdateCpuPressure(System &system)
    {
        const auto now = std::chrono::steady_clock::now();
        if (now - m_last_cpu_sample < std::chrono::milliseconds(500))
            return;
        m_last_cpu_sample = now;

        const auto &cpu_info = system.device_system.cpu_system.GetCPUInfo();
        float cpu_0to1 = cpu_info.system_usage_percentage / 100.0f;
        fix_update_job_executor.UpdateCpuPressure(
            cpu_0to1,
            static_cast<size_t>(cpu_info.logical_processor_count),
            static_cast<size_t>(cpu_info.physical_core_count));
    }
}