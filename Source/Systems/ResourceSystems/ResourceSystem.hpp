#pragma once
#include "ObjectSystem.hpp"
#include "BehaviorSystem.hpp"
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

            Core::Config::ObjectIDType id = object_system.Add(std::forward<U>(object));

            auto &obj = object_system.Get(id);
            obj.static_behaviors.UploadBehaviors(id, behavior_system);

            return id;
        }

    };
}