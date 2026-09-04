#pragma once
#include <cstdint>
#include "Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Behaviors/BaseBehavior/BehaviorSet.hpp"
#include <vector>

#define BIND_BEHAVIORS(...) \
public: \
    using BindBehaviors = ::RandEngine::Core::Behaviors::BindBehaviorSet<__VA_ARGS__>;

namespace RandEngine::Core::Objects
{
    struct BaseObject
    {
        int64_t id = 0;

        BaseObject();
        virtual ~BaseObject();
    };
}