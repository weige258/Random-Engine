#pragma once

namespace RandomEngine::Core::Systems {
    struct System; 
}

namespace RandomEngine::Core::Behaviors
{
    struct ISystemUpdateBehavior{
          virtual void SystemUpdate(Systems::System& system)=0;

          virtual ~ISystemUpdateBehavior() = default;
    };
}