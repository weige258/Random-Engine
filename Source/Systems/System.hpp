#pragma once
#include "ISystem.hpp"
#include "Systems/ResourceSystems/ResourceSystem.hpp"
#include "Systems/DeviceSystems/DeviceSystem.hpp"
#include "Systems/RuntimeSystems/RuntimeSystem.hpp"


namespace RandomEngine::Systems{
    

struct System:Systems::ISystem {
    
    RandomEngine::Systems::DeviceSystems::DeviceSystem device_system ;
    RandomEngine::Systems::ResourceSystems::ResourceSystem resource_system ;
    RandomEngine::Systems::RuntimeSystems::RuntimeSystem runtime_system ;
    
    void Init(System &system);

    void Run(System &system);

    void Destroy();
};

}