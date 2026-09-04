#pragma once

#include "Core/Jobs/JobExecutor/LogicBehaviorJobExecutor.hpp"
#include "Systems/ISystem.hpp"

namespace RandomEngine::Systems
{
    struct System;
}

namespace RandomEngine::Systems::RuntimeSystems
{
    class LogicUpdateSystem : public ISystem
    {
    private:
        Core::Jobs::JobExecutor::LogicBehaviorJobExecutor logic_job_executor;

    public:
        void ApplyBehaviorChanges(
            const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> &added,
            const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> &deleted);

        void Init(System &system);
        void Run(System &system);
        void Destroy();
    };
}