#pragma once

#include "Systems/ResourceSystems/ResourceSystem.hpp"
#include "Systems/DeviceSystems/DeviceSystem.hpp"

namespace RandEngine::Systems{
    

struct System{
    
    RandEngine::Systems::DeviceSystems::DeviceSystem device_system ;
    RandEngine::Systems::ResourceSystems::ResourceSystem resource_system ;
    
    
    void Init();

    void Run();

    void Destroy();
};

}