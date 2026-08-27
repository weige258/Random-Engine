#pragma once

#include "Core/Job/LogicBehaviorJobWorker.hpp"
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
        Core::Job::LogicBehaviorJobWorker logic_behavior_job_worker;

    public:
        void ApplyBehaviorChanges(
            const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> &added,
            const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> &deleted);

        void Init(System &system);

        void Run(System &system);

        void Destroy();
    };
}