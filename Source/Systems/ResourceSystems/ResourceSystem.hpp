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
    };
}