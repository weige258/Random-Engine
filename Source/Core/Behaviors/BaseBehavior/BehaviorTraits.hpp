// Source/Core/Behaviors/BehaviorTraits.hpp
#pragma once

#include "BindBaseBehavior.hpp"

namespace RandomEngine::Core::Behaviors 
{
    /**
     * @brief 编译期行为特征提取器 (Trait)
     * @details 默认情况下，任何对象都没有绑定行为。
     *          通过 BIND_BEHAVIORS_TO_OBJECT 宏在外部进行特化绑定。
     */
    template <typename TObject>
    struct BehaviorsBindForObject 
    {
        using Type = BindBehaviorSet<>; // 空集合，合法且安全
    };
}

/**
 * @brief 分布式绑定语法糖宏
 * @note 必须在命名空间外部定义，避免宏展开时产生命名空间嵌套错误
 * @example BIND_BEHAVIORS_TO_OBJECT(PlayerObject, MovementBehavior, CombatBehavior)
 */
#define BIND_BEHAVIORS_TO_OBJECT(ObjectType, ...) \
    template <> \
    struct ::RandomEngine::Core::Behaviors::BehaviorsBindForObject<ObjectType> { \
        using Type = ::RandomEngine::Core::Behaviors::BindBehaviorSet<__VA_ARGS__>; \
        static_assert( \
            ::RandomEngine::Core::Behaviors::BindBehaviorSet<__VA_ARGS__>::count >= 0, \
            "BIND_BEHAVIORS_TO_OBJECT Error: All behaviors must derive from BindBaseBehavior!" \
        ); \
    };