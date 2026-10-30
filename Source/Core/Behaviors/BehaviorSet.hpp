#pragma once
#include "BaseBehavior/BaseBehavior.hpp"
#include "BaseBehavior/BaseBindBehavior.hpp"


namespace RandomEngine::Core::Behaviors
{

    template <typename BaseBehaviorRequire,typename... Behaviors>
        requires(std::is_base_of_v<BaseBehaviorRequire, Behaviors> && ...)
    struct BehaviorSet
    {
        static constexpr size_t count = sizeof...(Behaviors);

        template <typename F>
        static constexpr void ForEach(F &&f)
        {
            (f.template operator()<Behaviors>(), ...);
        }
    };


    template <typename... Behaviors>
    using BindBehaviorSet = BehaviorSet<BaseBindBehavior, Behaviors...>;
}