#pragma once
#include "ObjectSystems/ObjectSystem.hpp"
#include "BehaviorSystems/BehaviorSystem.hpp"
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems::ResourceSystems {
    
    class ResourceSystem:Systems::ISystem {
        
    public:    
        ObjectSystems::ObjectSystem object_system;
        BehaviorSystems::BehaviorSystem behavior_system;

        void Init();

        void Run(System& system);

        void Destroy();
    }; 
}