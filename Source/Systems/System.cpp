#include "System.hpp"

namespace RandomEngine::Systems
{
    void System::Init(System& system)
    {

        device_system.Init(system);
        resource_system.Init(system);
        runtime_system.Init(system);
    }

    void System::Run(System& system)
    {
        device_system.Run(system);
        resource_system.Run(system);
        runtime_system.Run(system);
        
    }

    void System::Destroy()
    {
        runtime_system.Destroy();
        resource_system.Destroy();    
        device_system.Destroy();    
    }
}