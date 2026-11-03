#include "BehaviorSystem.hpp"

namespace RandomEngine::Systems::ResourceSystems
{

        bool BehaviorSystem::DeleteBehavior(
            const Core::Memory::ObserverPtr<
                Core::Behaviors::BaseBehavior> &behavior)
        {
            if (!behavior)
                return false;

            auto *target = behavior.Get();

            if (!target)
                return false;

            std::unique_lock<std::shared_mutex> lock(
                m_structure_mutex);

            // 以裸指针为键直接命中行,均摊 O(1),不再全表扫描
            BehaviorRow *found_row = all_behaviors.FindRow(target);

            if (!found_row)
                return false;

            // 必须先摘行:此刻行内 MasterPtr 仍有效,索引才能按正确键移除
            if (!all_behaviors.Delete(found_row))
                return false;

            // 再从死行搬走所有权,由延迟队列持有到 RuntimeSystem 消费完
            auto &owner =
                found_row->template Get<
                    Core::Memory::MasterPtr<
                        Core::Behaviors::BaseBehavior>>();

            retired_behaviors.push_back(std::move(owner));
            behaviors_should_delete.emplace_back(retired_behaviors.back());

            return true;
        }

        bool BehaviorSystem::DeleteObjectBehaviors(const Core::Objects::ObjectId &object_id)
        {
            std::unique_lock<std::shared_mutex> lock(m_structure_mutex);

            auto rows = all_behaviors.FindRows(object_id);
            if (rows.empty())
                return false;

            for (auto *row : rows)
            {
                // 必须先摘行:此刻行内 MasterPtr 仍有效,索引才能按正确键移除
                if (!all_behaviors.Delete(row))
                    continue;

                // 再从死行搬走所有权,由延迟队列持有到 RuntimeSystem 消费完
                auto &owner =
                    row->template Get<
                        Core::Memory::MasterPtr<
                            Core::Behaviors::BaseBehavior>>();

                retired_behaviors.push_back(std::move(owner));
                behaviors_should_delete.emplace_back(retired_behaviors.back());
            }

            return true;
        }

        void BehaviorSystem::FlushPendingBehaviorsQueue()
        {
            behaviors_should_add.clear();
            behaviors_should_delete.clear();
            retired_behaviors.clear();
        }

        void BehaviorSystem::Init(System &system) {};

        void BehaviorSystem::Run(System &system) {

        };

        void BehaviorSystem::Destroy() {};
}