#include "DeviceSystem.hpp"

namespace RandEngine::Systems::DeviceSystems
{
     void DeviceSystem::Init(System &system)
     {
          os_system = DeviceSystems::OSSystem();
          os_system.Init();

          cpu_system = DeviceSystems::CPUSystem();
          cpu_system.Init();

          window_system = DeviceSystems::WindowSystem();
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