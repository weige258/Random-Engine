#include "DeviceSystem.hpp"

namespace RandEngine::Core::Systems::DeviceSystems
{
     void DeviceSystem::Init()
     {
          os_device = DeviceSystems::OSDevices::OSDevice();
          os_device.Init();

          window_device = DeviceSystems::WindowDevices::WindowDevice();
          window_device.Init();
     }

     void DeviceSystem::Run()
     {
          window_device.Run();
     
     }

     void DeviceSystem::Destroy()
     {
          window_device.Destroy();
     }

}