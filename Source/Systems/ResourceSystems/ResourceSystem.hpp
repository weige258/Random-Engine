#pragma once
#include "ObjectSystem.hpp"
#include "BehaviorSystem.hpp"
#include "Behaviors/BaseBehavior/BaseBindBehavior.hpp"
#include "Systems/ISystem.hpp"
#include "Core/Behaviors/BehaviorTraits.hpp"
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

            using Behaviors = typename RandomEngine::Core::Behaviors::BehaviorsBindForObject<std::decay_t<U>>::Type;
            Behaviors::ForEach([&]<typename B>()
                               {
        auto behavior = Core::Memory::MasterPtr<B>(new B());
        behavior->bind_object_id = id;
        behavior_system.AddBehavior(std::move(behavior)); });

            return id;
        }
    };
}