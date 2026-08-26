#pragma once
#include "BehaviorChain.hpp"
#include "Memory/MasterPtr.hpp"
#include "Memory/ObserverPtr.hpp"
#include "BindBaseBehavior.hpp"
#include <algorithm>
#include <type_traits>

namespace RandEngine::Core::Behaviors
{
    struct DynamicBehaviors : public BehaviorChain
    {
        DynamicBehaviors() = default;
        ~DynamicBehaviors() = default;

        // 统一 Move-Only 语义：禁用拷贝，允许移动
        DynamicBehaviors(const DynamicBehaviors &) = delete;
        DynamicBehaviors &operator=(const DynamicBehaviors &) = delete;

        DynamicBehaviors(DynamicBehaviors &&) noexcept = default;
        DynamicBehaviors &operator=(DynamicBehaviors &&) noexcept = default;

        template <typename... Args>
            requires(sizeof...(Args) > 0) &&
                    ((std::is_base_of_v<BindBaseBehavior, std::decay_t<Args>> ||
                      std::is_convertible_v<Args, Memory::MasterPtr<BindBaseBehavior>>) &&
                     ...)
        DynamicBehaviors(Args &&...args)
        {
            // 自动折叠展开转换
            (m_staging_behaviors.push_back(ToMasterPtr(std::forward<Args>(args))), ...);
        }

        template <typename T>
            requires std::is_base_of_v<BindBaseBehavior, T>
        Core::Memory::ObserverPtr<T> Add(Core::Memory::MasterPtr<T> behavior)
        {
            if (!behavior) return {};
            Core::Memory::ObserverPtr<T> observer(behavior);
            m_staging_behaviors.push_back(std::move(behavior));
            return observer;
        }

        template <typename T>
            requires std::is_base_of_v<BindBaseBehavior, T>
        [[nodiscard]] Core::Memory::ObserverPtr<T> Get() const
        {
            // 先在已生效的 Observer 队列中查找
            for (const auto &obs : m_active_observers)
            {
                if (obs && dynamic_cast<T *>(obs.Get()) != nullptr)
                {
                    return Core::Memory::dynamic_observer_cast<T>(obs);
                }
            }
            // 若还未 Upload，在 Staging 暂存队列中查找
            for (const auto &master : m_staging_behaviors)
            {
                if (master && dynamic_cast<T *>(master.Get()) != nullptr)
                {
                    return Core::Memory::ObserverPtr<T>(*reinterpret_cast<const Core::Memory::MasterPtr<T>*>(&master));
                }
            }
            return {};
        }

        template <typename T>
            requires std::is_base_of_v<BindBaseBehavior, T>
        [[nodiscard]] bool Has() const
        {
            return static_cast<bool>(Get<T>());
        }

        template <typename T>
            requires std::is_base_of_v<BindBaseBehavior, T>
        bool Remove()
        {
            bool removed = false;

            // 清理 Staging 队列
            auto staging_it = std::remove_if(m_staging_behaviors.begin(), m_staging_behaviors.end(),
                [](const auto &master) {
                    return master && dynamic_cast<T *>(master.Get()) != nullptr;
                });
            if (staging_it != m_staging_behaviors.end())
            {
                m_staging_behaviors.erase(staging_it, m_staging_behaviors.end());
                removed = true;
            }

            // 清理 Active Observer 队列
            auto active_it = std::remove_if(m_active_observers.begin(), m_active_observers.end(),
                [](const auto &obs) {
                    return obs && dynamic_cast<T *>(obs.Get()) != nullptr;
                });
            if (active_it != m_active_observers.end())
            {
                m_active_observers.erase(active_it, m_active_observers.end());
                removed = true;
            }

            return removed;
        }
    };
}