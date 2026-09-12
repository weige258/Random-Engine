#pragma once

#include "Config.hpp"

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Behaviors
{
    struct ILogicUpdateBehavior 
    {
        virtual void LogicUpdate(Config::TimeType delta_time,RandomEngine::Systems::System& system)=0;

        virtual ~ILogicUpdateBehavior() = default;
    }; 
}