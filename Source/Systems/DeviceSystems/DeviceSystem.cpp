#include "DeviceSystem.hpp"

namespace RandEngine::Systems::DeviceSystems
{
     void DeviceSystem::Init()
     {
          os_system = DeviceSystems::OSSystems::OSSystem();
          os_system.Init();

          cpu_system = DeviceSystems::CPUSystems::CPUSystem();
          cpu_system.Init();

          window_system = DeviceSystems::WindowSystems::WindowSystem();
          window_system.Init();
     }

     void DeviceSystem::Run(System& system)
     {
          window_system.Run();
          cpu_system.Run();
     }

     void DeviceSystem::Destroy()
     {
          window_system.Destroy();
          cpu_system.Destroy();
     }

}