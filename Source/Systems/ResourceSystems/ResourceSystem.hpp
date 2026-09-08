#pragma once
#include "ObjectSystem.hpp"
#include "BehaviorSystem.hpp"
#include "Behaviors/BaseBehavior/BindBaseBehavoir.hpp"
#include "Systems/ISystem.hpp"


namespace RandomEngine::Systems::ResourceSystems
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
            auto id = object_system.Add(std::forward<U>(object));

            using Behaviors = typename std::decay_t<U>::BindBehaviors;
            Behaviors::ForEach([&]<typename B>()
                               {
        auto behavior = Core::Memory::MasterPtr<B>(new B());
        behavior->bind_id = id;
        behavior_system.AddBehavior(std::move(behavior)); });

            return id;
        }
    };
}