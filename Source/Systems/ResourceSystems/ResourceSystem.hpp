#pragma once
#include "ObjectSystems/ObjectSystem.hpp"
#include "BehaviorSystems/BehaviorSystem.hpp"

namespace RandEngine::Systems::ResourceSystems {
    
    class ResourceSystem {
        
    public:    
        ObjectSystems::ObjectSystem object_system;
        BehaviorSystems::BehaviorSystem behavior_system;

        void Init();

        void Run();

        void Destroy();
    }; 
}