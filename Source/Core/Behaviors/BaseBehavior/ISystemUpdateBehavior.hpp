#pragma once

namespace RandomEngine::Systems {
    struct System; 
}

namespace RandomEngine::Core::Behaviors
{
    struct ISystemUpdateBehavior{
          virtual void SystemUpdate(RandomEngine::Systems::System& system)=0;

          virtual ~ISystemUpdateBehavior() = default;
    };
}