#pragma once

#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include "Systems/ISystem.hpp"
#include <memory>
#include <vector>

namespace RandEngine::Systems::ResourceSystems::BehaviorSystems
{

    class BehaviorSystem : Systems::ISystem
    {
    private:
        std::vector<Core::Memory::MasterPtr<Core::Behaviors::BaseBehavior>> all_behaviors;

        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> logic_behavior_should_add;
        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> logic_behavior_should_delete;

    public:
        template <typename T>
        void AddBehavior(Core::Memory::MasterPtr<T> behavior)
        {
            if (!behavior)
                return;

            // 如果实现了 ILogicUpdateBehavior，压入待添加队列
            if constexpr (std::is_base_of_v<Core::Behaviors::ILogicUpdateBehavior, T>)
            {
                logic_behavior_should_add.push_back(behavior.Get());
            }

            all_behaviors.push_back(std::move(behavior));
        }

        template <typename T>
        void DeleteBehavior(Core::Memory::ObserverPtr<T> behavior)
        {
            if (!behavior)
                return;

            // 编译期检查：如果 T 继承自 ILogicUpdateBehavior，自动隐式转换并压入
            if constexpr (std::is_base_of_v<Core::Behaviors::ILogicUpdateBehavior, T>)
            {
                logic_behavior_should_delete.push_back(behavior);
            }

            std::erase_if(all_behaviors, [&](const auto &master)
                          { return master.Get() == behavior.Get(); });
        }

        void Init() {};

        void Run(System &system) {

        };

        void Destroy() {};
    };
}
