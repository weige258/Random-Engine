#pragma once

#include "Systems/ResourceSystems/ResourceSystem.hpp"
#include "Systems/DeviceSystems/DeviceSystem.hpp"

namespace RandEngine::Core::Systems{
    

struct System{
    
    RandEngine::Core::Systems::DeviceSystems::DeviceSystem device_system ;
    RandEngine::Core::Systems::ResourceSystems::ResourceSystem resource_system ;
    
    
    void Init();

    void Run();

    void Destroy();
};

}