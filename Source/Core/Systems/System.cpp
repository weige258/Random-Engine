#include "System.hpp"

namespace RandEngine::Core::Systems
{
    void System::Init()
    {
        platform_system = PlatformSystems::PlatformSystem();
        platform_system.Init();

        resource_system = ResourceSystems::ResourceSystem();
        resource_system.Init();
    }

    void System::Run()
    {
        platform_system.Run();
        resource_system.Run();
        
    }

    void System::Destory()
    {
        resource_system.Destroy();    
        platform_system.Destroy();    
    }
}