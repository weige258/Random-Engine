#pragma once

namespace RandEngine::Systems { struct System; }

namespace RandEngine::Core::Behaviors
{
    struct ILogicUpdateBehavior 
    {
        virtual void LogicUpdate(float delta_time,RandEngine::Systems::System& system)=0;

        virtual ~ILogicUpdateBehavior() = default;
    }; 
}