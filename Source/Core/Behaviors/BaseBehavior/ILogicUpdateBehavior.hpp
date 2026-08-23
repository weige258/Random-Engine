#pragma once
namespace RandEngine::Core::Behaviors
{
    struct ILogicUpdateBehavior 
    {
        virtual void LogicUpdate(const float& delta_time)=0;

        virtual ~ILogicUpdateBehavior() = default;
    }; 
}