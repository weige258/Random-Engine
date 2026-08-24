#include "ResourceSystem.hpp"
#include "Time/Timer.hpp"
#include "iostream"
namespace RandEngine::Systems::ResourceSystems {
    
    Core::Time::Timer timer;
    
    void ResourceSystem::Init(){
       object_system.Init();

       timer.Start();
    }

    void ResourceSystem::Run(System& system){
       object_system.Run(system);
       behavior_system.Run(system);
       std::cout << timer.GetDeltaTime() << std::endl;
    }

    void ResourceSystem::Destroy(){
        object_system.Destroy();
    }
}