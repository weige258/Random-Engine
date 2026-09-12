#pragma once
#include "BaseBehavior.hpp"
#include "Config.hpp"

namespace RandomEngine::Core::Behaviors {
    
    struct BindBaseBehavior: BaseBehavior
    {
        static constexpr Config::ObjectIDType null_index = static_cast<Config::ObjectIDType>(-1);
        Config::ObjectIDType bind_id=null_index;

        BindBaseBehavior();
        virtual ~BindBaseBehavior();
    };
    
}