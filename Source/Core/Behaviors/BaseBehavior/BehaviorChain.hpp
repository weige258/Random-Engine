#pragma once
#include "Memory/MasterPtr.hpp"
#include "Memory/ObserverPtr.hpp"
#include "Core/Behaviors/BaseBehavior/BindBaseBehavior.hpp"
#include "Systems/ResourceSystems/BehaviorSystem.hpp"
#include <deque>

namespace RandEngine::Core::Behaviors
{

    struct BehaviorChain
    {
    protected:
        std::deque<Core::Memory::MasterPtr<Core::Behaviors::BindBaseBehavior>> m_staging_behaviors;
        std::deque<Core::Memory::ObserverPtr<Core::Behaviors::BindBaseBehavior>> m_active_observers;

    public:
        BehaviorChain() = default;
        ~BehaviorChain() = default;

        BehaviorChain(const BehaviorChain &) = delete;
        BehaviorChain &operator=(const BehaviorChain &) = delete;

        BehaviorChain(BehaviorChain &&) noexcept = default;
        BehaviorChain &operator=(BehaviorChain &&) noexcept = default;

        void SetBindID(Core::Config::ObjectIDType id)
        {
            for (auto &master : m_staging_behaviors)
            {
                if (!master)
                {
                    continue;
                }
                master->bind_id = id;
            }
        }

        void UploadBehaviors(Core::Config::ObjectIDType id, RandEngine::Systems::ResourceSystems::BehaviorSystem &behavior_system)
        {
            SetBindID(id);

            for (auto &master : m_staging_behaviors)
            {
                if (!master)
                {
                    continue;
                }

                m_active_observers.push_back(Core::Memory::ObserverPtr<Core::Behaviors::BindBaseBehavior>(master));
                behavior_system.AddBehavior(std::move(master));
            }

            std::erase_if(m_staging_behaviors, [](const auto &master)
                          { return !master; });

            std::erase_if(m_active_observers, [](const auto &observer)
                          { return !observer; });
        }
    };
}