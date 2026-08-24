#include "System.hpp"

namespace RandEngine::Systems
{
    void System::Init()
    {
        device_system = DeviceSystems::DeviceSystem();
        device_system.Init();

        resource_system = ResourceSystems::ResourceSystem();
        resource_system.Init();
    }

    void System::Run(System& system)
    {
        device_system.Run(system);
        resource_system.Run(system);
        
    }

    void System::Destroy()
    {
        resource_system.Destroy();    
        device_system.Destroy();    
    }
}