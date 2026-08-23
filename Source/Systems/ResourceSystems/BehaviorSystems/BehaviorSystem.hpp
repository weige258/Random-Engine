#pragma once

#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include <memory>
#include <vector>

namespace RandEngine::Systems::ResourceSystems::BehaviorSystems
{

    class BehaviorSystem
    {
    private:
        std::vector<Core::Memory::MasterPtr<Core::Behaviors::BaseBehavior>> all_behaviors;

    public:
        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::ILogicUpdateBehavior>> logic_update_behaviors;

        template <typename T>
        void AddBehavior(Core::Memory::MasterPtr<T> behavior)
        {
            if (!behavior)
                return;

            if constexpr (std::is_base_of_v<Core::Behaviors::ILogicUpdateBehavior, T>)
            {
                logic_update_behaviors.emplace_back(behavior);
            }

            all_behaviors.push_back(std::move(behavior));
        }

        void DeleteBehavior(Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior> behavior)
        {
            if (!behavior)
                return;

            std::erase_if(logic_update_behaviors, [&](const auto &ptr)
                          { return ptr.Get() == dynamic_cast<Core::Behaviors::ILogicUpdateBehavior *>(behavior.Get()); });

            std::erase_if(all_behaviors, [&](const auto &master)
                          { return master.Get() == behavior.Get(); });
        }
    };

}