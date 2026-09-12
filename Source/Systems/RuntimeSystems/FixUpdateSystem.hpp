#pragma once

#include "Core/Jobs/JobExecutor/FixUpdateJobExecutor.hpp"
#include "Core/Behaviors/BaseBehavior/IFixUpdateBehavior.hpp"
#include "Core/Config.hpp"
#include "Systems/ISystem.hpp"


namespace RandomEngine::Systems::RuntimeSystems{
    class FixUpdateSystem : public ISystem{
        private:
            RandomEngine::Core::Config::TimeType fixed_delta_time = RandomEngine::Core::Config::TimeType(1)/RandomEngine::Core::Config::TimeType(30);
            Core::Jobs::JobExecutor::FixUpdateJobExecutor fix_update_job_executor;
            std::chrono::steady_clock::time_point m_last_cpu_sample;

        public:
            void ApplyBehaviorChanges(
                const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::IFixUpdateBehavior>>& added,
                const std::vector<Core::Memory::ObserverPtr<Core::Behaviors::IFixUpdateBehavior>>& deleted);

            void Init(System& system);

            void Run(System& system);

            void Destroy();

            void UpdateCpuPressure(System& system);
    };
}