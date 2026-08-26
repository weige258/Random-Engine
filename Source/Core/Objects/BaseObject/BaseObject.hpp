#pragma once
#include <cstdint>
#include "Behaviors/BaseBehavior/StaticBehaviors.hpp"
#include "Behaviors/BaseBehavior/DynamicBehaviors.hpp"

namespace RandEngine::Core::Objects
{
    struct BaseObject
    {
        int64_t id = 0;
        Behaviors::StaticBehaviors static_behaviors;
        Behaviors::DynamicBehaviors dynamic_behaviors;

        BaseObject() ;
        virtual ~BaseObject() ;

        BaseObject(const BaseObject &)=delete ;
        BaseObject &operator=(const BaseObject &)=delete;

        BaseObject(BaseObject &&) noexcept = default;
        BaseObject &operator=(BaseObject &&) noexcept = default;
    };
}