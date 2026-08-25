#include "ResourceSystem.hpp"
#include "Time/Timer.hpp"
#include "iostream"
namespace RandEngine::Systems::ResourceSystems {
    
    void ResourceSystem::Init(System& system){
       object_system.Init(system);
       behavior_system.Init(system);
    }

    void ResourceSystem::Run(System& system){
       object_system.Run(system);
       behavior_system.Run(system);
    }

    void ResourceSystem::Destroy(){
        object_system.Destroy();
        behavior_system.Destroy();
    }
}