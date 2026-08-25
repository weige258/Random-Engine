#pragma once

#include "Core/Job/LogicBehaviorJobWorker.hpp"
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems { struct System; }

namespace RandEngine::Systems::RuntimeSystems
{

    class LogicUpdateSystem : public ISystem
    {
    private:
        Core::Job::LogicBehaviorJobWorker logic_behavior_job_worker;
        size_t update_rate=60;
        int count=0;
    
        void UploadLogicBehaviorsFromBehaviorSystem(System& system);

    public:
        void Init(System& system);

        void Run(System &system);

        void Destroy();
    };
}