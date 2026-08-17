#pragma once
#include "Systems/ResourceSystems/ObjectSystems/ObjectSystem.hpp"

namespace RandEngine::Core::Systems::ResourceSystems {
    
    class ResourceSystem {
        
    public:    
        ObjectSystems::ObjectSystem object_system;

        void Init();

        void Run();

        void Destroy();
    }; 
}