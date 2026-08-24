#pragma once

#include "Systems/ResourceSystems/ResourceSystem.hpp"
#include "Systems/DeviceSystems/DeviceSystem.hpp"
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems{
    

struct System:Systems::ISystem{
    
    RandEngine::Systems::DeviceSystems::DeviceSystem device_system ;
    RandEngine::Systems::ResourceSystems::ResourceSystem resource_system ;
    
    
    void Init();

    void Run(System& system);

    void Destroy();
};

}