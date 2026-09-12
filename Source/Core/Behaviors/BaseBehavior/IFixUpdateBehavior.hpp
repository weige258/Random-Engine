#pragma once

#include "Config.hpp"

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Behaviors{

    struct IFixUpdateBehavior{
        virtual void FixUpdate(Config::TimeType delta_time,RandomEngine::Systems::System& system) = 0;

        virtual ~IFixUpdateBehavior() = default;
    };
}