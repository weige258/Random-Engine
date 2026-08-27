#include "RuntimeSystem.hpp"
#include "Systems/System.hpp"

namespace RandEngine::Systems::RuntimeSystems
{ 
    void RuntimeSystem::Init(Systems::System &system){
        logic_update_system.Init(system);
    }

    void RuntimeSystem::Run(Systems::System &system){
        logic_update_system.Run(system);


        system.resource_system.behavior_system.FlushPendingQueue();
    }

    void RuntimeSystem::Destroy(){
         logic_update_system.Destroy();
    }
    
}