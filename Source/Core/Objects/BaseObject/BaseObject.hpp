#pragma once
#include <cstdint>
#include "Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include <vector>

namespace RandEngine::Core::Objects
{
    struct BaseObject
    {
        int64_t id = 0;
        
        BaseObject() ;
        virtual ~BaseObject() ;
    };
}