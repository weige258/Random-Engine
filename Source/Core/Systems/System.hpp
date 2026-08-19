#pragma once

#include "Systems/ResourceSystems/ResourceSystem.hpp"
#include "Systems/PlatformSystems/PlatformSystem.hpp"

namespace RandEngine::Core::Systems{
    

struct System{
    
    RandEngine::Core::Systems::PlatformSystems::PlatformSystem platform_system ;
    RandEngine::Core::Systems::ResourceSystems::ResourceSystem resource_system ;
    
    
    void Init();

    void Run();

    void Destory();
};

}