#pragma once

#include "BaseObject.hpp"
#include "Behaviors/BaseBehavior/StaticBehaviors.hpp"
#include "Behaviors/BaseBehavior/DynamicBehaviors.hpp"

namespace RandEngine::Core::Objects {
    
    template <typename... Behaviors>
    struct BindStaticBehaviors : virtual public BaseObject
    {
        BindStaticBehaviors()
        {
            static_behaviors = Core::Behaviors::StaticBehaviors(Behaviors{}...);
        }
    };

    template <typename... Behaviors>
    struct BindDynamicBehaviors : virtual public BaseObject
    {
        BindDynamicBehaviors()
        {
            dynamic_behaviors = Core::Behaviors::DynamicBehaviors(Behaviors{}...);
        }
    };


}