#pragma once

namespace RandEngine::Core::Behaviors
{
    struct ISystemUpdateBehavior{
          virtual void SystemUpdate()=0;

          virtual ~ISystemUpdateBehavior() = default;
    };
}