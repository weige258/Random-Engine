#include "ResourceSystem.hpp"

namespace RandEngine::Core::Systems::ResourceSystems {
    
    
    void ResourceSystem::Init(){
       object_system.Init();
    }

    void ResourceSystem::Run(){
       object_system.Run();
    }

    void ResourceSystem::Destroy(){
        object_system.Destroy();
    }
}