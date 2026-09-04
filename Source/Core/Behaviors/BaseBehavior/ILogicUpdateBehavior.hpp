#pragma once

namespace RandomEngine::Systems { struct System; }

namespace RandomEngine::Core::Behaviors
{
    struct ILogicUpdateBehavior 
    {
        virtual void LogicUpdate(float delta_time,RandomEngine::Systems::System& system)=0;

        virtual ~ILogicUpdateBehavior() = default;
    }; 
}