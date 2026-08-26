#pragma once
#include "BehaviorChain.hpp"
#include "Memory/MasterPtr.hpp"
#include "BindBaseBehavior.hpp"
#include "Objects/BaseObject/BaseObject.hpp"

namespace RandEngine::Core::Behaviors
{
    namespace
    {
        template <typename T>
        static Memory::MasterPtr<BindBaseBehavior> ToMasterPtr(T &&arg)
        {
            using RawT = std::decay_t<T>;

            if constexpr (std::is_base_of_v<BindBaseBehavior, RawT>)
            {
                return Memory::MakeMaster<RawT>(std::forward<T>(arg));
            }
            else
            {
                return std::forward<T>(arg);
            }
        }
    }

    struct StaticBehaviors : public BehaviorChain
    {

        StaticBehaviors() = default;
        ~StaticBehaviors() = default;
        
        StaticBehaviors(const StaticBehaviors &) = delete;
        StaticBehaviors &operator=(const StaticBehaviors &) = delete;
        StaticBehaviors(StaticBehaviors &&) noexcept = default;
        StaticBehaviors &operator=(StaticBehaviors &&) noexcept = default;

        template <typename... Args>
            requires(sizeof...(Args) > 0) &&
                    ((std::is_base_of_v<BindBaseBehavior, std::decay_t<Args>> ||
                      std::is_convertible_v<Args, Memory::MasterPtr<BindBaseBehavior>>) &&
                     ...)
        StaticBehaviors(Args &&...args)
        {
            // 自动折叠展开转换
            (m_staging_behaviors.push_back(ToMasterPtr(std::forward<Args>(args))), ...);
        }
    };
}