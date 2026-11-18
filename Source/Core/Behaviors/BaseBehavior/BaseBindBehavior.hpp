#pragma once
#include "BaseBehavior.hpp"
#include "Objects/BaseObject/ObjectId.hpp"
#include "Config.hpp"

namespace RandomEngine::Core::Behaviors {
    
    struct BaseBindBehavior: BaseBehavior
    {
        static constexpr Config::ObjectIdType null_index = static_cast<Config::ObjectIdType>(-1);
        Objects::ObjectId bind_object_id=null_index;

        BaseBindBehavior();
        virtual ~BaseBindBehavior();
    };
    
}