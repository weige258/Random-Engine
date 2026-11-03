#pragma once

#include "Core/Objects/BaseObject/ObjectId.hpp"
#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include "Systems/ISystem.hpp"
#include "Core/Containers/StableRowTable.hpp"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <utility>

namespace RandomEngine::Systems::ResourceSystems
{

    class BehaviorSystem : Systems::ISystem
    {
    private:
        using BehaviorTable = Core::Containers::StableRowTable<
            Core::Memory::MasterPtr<Core::Behaviors::BaseBehavior>,
            std::optional<Core::Objects::ObjectId>>;
        using BehaviorRow = BehaviorTable::Row;

        BehaviorTable all_behaviors;

        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>> behaviors_should_add;
        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>> behaviors_should_delete;
        // 持有被删行为的所有权,直到 RuntimeSystem 消费完通知才真正析构
        std::vector<Core::Memory::MasterPtr<Core::Behaviors::BaseBehavior>> retired_behaviors;

    private:
        mutable std::shared_mutex m_structure_mutex;

    public:
        // 添加行为
        template <typename T>
        void AddBehavior(Core::Memory::MasterPtr<T> behavior)
        {
            if (!behavior)
                return;

            std::unique_lock<std::shared_mutex> struct_lock(m_structure_mutex);

            behaviors_should_add.push_back(Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>(behavior));

            if constexpr (std::is_base_of_v<Core::Behaviors::BaseBindBehavior, T>)
            {
                // 先取 id 再 move,避免实参求值顺序导致的 use-after-move
                const auto bind_object_id = behavior->bind_object_id;
                all_behaviors.Insert(std::move(behavior), bind_object_id);
            }
            else if constexpr (std::is_base_of_v<Core::Behaviors::BaseBehavior, T>)
            {
                all_behaviors.Insert(std::move(behavior), std::nullopt);
            }
        }

        // 删除行为
        bool DeleteBehavior(const Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior> &behavior);
        
        bool DeleteObjectBehaviors(const Core::Objects::ObjectId &object_id);
        

        template <typename T>
        std::pair<std::vector<Core::Memory::ObserverPtr<T>>,
                  std::vector<Core::Memory::ObserverPtr<T>>>
        FetchBehaviorsChangesToRuntimeSystem() const
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

        void FlushPendingBehaviorsQueue();
  
        void Init(System &system);

        void Run(System &system);

        void Destroy();
    };
}