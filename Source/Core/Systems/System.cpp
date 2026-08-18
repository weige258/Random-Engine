#include "System.hpp"

namespace RandEngine::Core::Systems
{
    void System::Init()
    {
        resource_system = ResourceSystems::ResourceSystem();
        resource_system.Init();
    }

    void System::Run()
    {
        resource_system.Run();
    }

    void System::Destory()
    {
        resource_system.Destroy();        
    }
}