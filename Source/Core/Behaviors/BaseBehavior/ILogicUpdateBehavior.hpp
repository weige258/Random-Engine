#pragma once

namespace RandEngine::Systems { struct System; }

namespace RandEngine::Core::Behaviors
{
    struct ILogicUpdateBehavior 
    {
        virtual void LogicUpdate(const float& delta_time,Systems::System& system)=0;

        virtual ~ILogicUpdateBehavior() = default;
    }; 
}