#pragma once
#include <cstdint>
#include "Behaviors/BaseBehavior/BindBaseBehavior.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Behaviors/BehaviorSet.hpp"
#include "Core/Objects/BaseObject/ObjectId.hpp"
#include <vector>

namespace RandomEngine::Core::Objects
{
    struct BaseObject
    {
        ObjectId id;
  
        BaseObject();
        virtual ~BaseObject();

        BaseObject(const BaseObject&) = delete;
        BaseObject& operator=(const BaseObject&) = delete;

        BaseObject(BaseObject&&) noexcept = default;
        BaseObject& operator=(BaseObject&&) noexcept = default;
    };
}