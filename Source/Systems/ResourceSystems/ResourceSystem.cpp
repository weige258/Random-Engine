#include "ResourceSystem.hpp"

#include "Job/Test.hpp"

namespace RandEngine::Systems::ResourceSystems {
    
    
    void ResourceSystem::Init(){
       Core::Job::RunWorkerBenchmark();
       object_system.Init();
    }

    void ResourceSystem::Run(){
       object_system.Run();
    }

    void ResourceSystem::Destroy(){
        object_system.Destroy();
    }
}