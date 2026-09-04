#pragma once

#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include "Systems/ISystem.hpp"
#include <memory>
#include <vector>
#include <utility>

namespace RandomEngine::Systems::ResourceSystems
{

    class BehaviorSystem : Systems::ISystem
    {
    private:
        std::vector<Core::Memory::MasterPtr<Core::Behaviors::BaseBehavior>> all_behaviors;

        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>> behaviors_should_add;
        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>> behaviors_should_delete;

    public:
        template <typename T>
        void AddBehavior(Core::Memory::MasterPtr<T> behavior)
        {
            if (!behavior)
                return;

            behaviors_should_add.push_back(Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>(behavior));

            all_behaviors.push_back(std::move(behavior));
        }

        template <typename T>
        std::pair<std::vector<Core::Memory::ObserverPtr<T>>,
                  std::vector<Core::Memory::ObserverPtr<T>>>
        FetchBehaviorChangesToRuntimeSystem() const
        {
            std::vector<Core::Memory::ObserverPtr<T>> matched_add;
            std::vector<Core::Memory::ObserverPtr<T>> matched_delete;

            for (const auto &item : behaviors_should_add)
            {
                if (auto casted = Core::Memory::dynamic_observer_cast<T>(item))
                {
                    matched_add.push_back(casted);
                }
            }

            for (const auto &item : behaviors_should_delete)
            {
                if (auto casted = Core::Memory::dynamic_observer_cast<T>(item))
                {
                    matched_delete.push_back(casted);
                }
            }

            return {std::move(matched_add), std::move(matched_delete)};
        }

        void FlushPendingQueue()
        {
            behaviors_should_add.clear();
            behaviors_should_delete.clear();
        }

        void Init(System &system) {};

        void Run(System &system) {

        };

        void Destroy() {};
    };
}