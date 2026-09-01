#pragma once
#include "BaseBehavior.hpp"
#include "BindBaseBehavoir.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include <vector>

namespace RandEngine::Core::Behaviors
{

    template <typename... Behaviors>
        requires(std::is_base_of_v<BindBaseBehavior, Behaviors> && ...)
    struct BehaviorSet
    {
        static constexpr size_t count = sizeof...(Behaviors);

        template <typename F>
        static constexpr void ForEach(F &&f)
        {
            (f.template operator()<Behaviors>(), ...);
        }
    };
}