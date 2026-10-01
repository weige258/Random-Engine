#pragma once
#include "BaseBehavior.hpp"
#include "Objects/BaseObject/ObjectId.hpp"
#include "Config.hpp"

namespace RandomEngine::Core::Behaviors {
    
    struct BindBaseBehavior: BaseBehavior
    {
        static constexpr Config::ObjectIDType null_index = static_cast<Config::ObjectIDType>(-1);
        Objects::ObjectId bind_object_id=null_index;

        BindBaseBehavior();
        virtual ~BindBaseBehavior();
    };
    
}