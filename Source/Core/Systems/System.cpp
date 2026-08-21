#include "System.hpp"

namespace RandEngine::Core::Systems
{
    void System::Init()
    {
        device_system = DeviceSystems::DeviceSystem();
        device_system.Init();

        resource_system = ResourceSystems::ResourceSystem();
        resource_system.Init();
    }

    void System::Run()
    {
        device_system.Run();
        resource_system.Run();
        
    }

    void System::Destroy()
    {
        resource_system.Destroy();    
        device_system.Destroy();    
    }
}