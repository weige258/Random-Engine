#pragma once
#include "BaseBehavior.hpp"
#include "BindBaseBehavoir.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include <vector>

namespace RandEngine::Core::Behaviors {

template <typename... Behaviors>
    requires (std::is_base_of_v<BindBaseBehavior, Behaviors> && ...)
struct BehaviorSet
{
    std::vector<Core::Memory::MasterPtr<BindBaseBehavior>> behaviors;

    BehaviorSet()
    {
        behaviors.reserve(sizeof...(Behaviors));
        (behaviors.emplace_back(new Behaviors()), ...);
    }

    operator std::vector<Core::Memory::MasterPtr<BindBaseBehavior>>() &&
    {
        return std::move(behaviors);
    }
};
}