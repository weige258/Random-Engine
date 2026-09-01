#pragma once

#include "Core/Job/AffinityJobExecutor.hpp"
#include "Core/Job/LogicBehaviorJobExecutor.hpp"
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems
{
    struct System;
}

namespace RandEngine::Systems::RuntimeSystems
{
    class LogicUpdateSystem : public ISystem
    {
    private:
        Core::Job::LogicBehaviorJobExecutor logic_job_executor;

    public:
        void ApplyBehaviorChanges(
            const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> &added,
            const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> &deleted);

        void Init(System &system);
        void Run(System &system);
        void Destroy();
    };
}