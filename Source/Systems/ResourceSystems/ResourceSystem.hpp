#pragma once
#include "ObjectSystem.hpp"
#include "BehaviorSystem.hpp"
#include "Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems::ResourceSystems
{

    class ResourceSystem : Systems::ISystem
    {

    public:
        ObjectSystem object_system;
        BehaviorSystem behavior_system;

        void Init(System &system);

        void Run(System &system);

        void Destroy();

        template <typename U>
        Core::Config::ObjectIDType Add(U &&object)
        {
            std::vector<Core::Memory::MasterPtr<Core::Behaviors::BindBaseBehavior>> behaviors = object.GetBehaviors();

            Core::Config::ObjectIDType id = object_system.Add(std::forward<U>(object));

            for (auto &behavior : behaviors)
            {
                if (!behavior)
                    continue;

                // 绑定实体 ID
                behavior->bind_id = id;

                // 移交所有权给 BehaviorSystem
                behavior_system.AddBehavior(std::move(behavior));
            }

            return id;
        }
    };
}