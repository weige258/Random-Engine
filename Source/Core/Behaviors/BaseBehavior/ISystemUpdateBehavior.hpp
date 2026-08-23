#pragma once

namespace RandEngine::Core::Systems {
    struct System; 
}

namespace RandEngine::Core::Behaviors
{
    struct ISystemUpdateBehavior{
          virtual void SystemUpdate(Systems::System& system)=0;

          virtual ~ISystemUpdateBehavior() = default;
    };
}