#pragma once
#include <cstdint>
#include "Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Behaviors/BaseBehavior/BehaviorSet.hpp"
#include <vector>

#define BIND_BEHAVIORS(...) \
    std::vector<::RandEngine::Core::Memory::MasterPtr<::RandEngine::Core::Behaviors::BindBaseBehavior>> GetBehaviors() override \
    { \
        return ::RandEngine::Core::Behaviors::BehaviorSet<__VA_ARGS__>{}; \
    }

namespace RandEngine::Core::Objects
{
    struct BaseObject
    {
        int64_t id = 0;

        BaseObject();
        virtual ~BaseObject();

        virtual std::vector<Memory::MasterPtr<Behaviors::BindBaseBehavior>> GetBehaviors();
    };
}