#include "DeviceSystem.hpp"

namespace RandEngine::Systems::DeviceSystems
{
     void DeviceSystem::Init()
     {
          os_device = DeviceSystems::OSDevices::OSDevice();
          os_device.Init();

          cpu_device = DeviceSystems::CPUDivices::CPUDevice();
          cpu_device.Init();

          window_device = DeviceSystems::WindowDevices::WindowDevice();
          window_device.Init();
     }

     void DeviceSystem::Run()
     {
          window_device.Run();
          cpu_device.Run();
     }

     void DeviceSystem::Destroy()
     {
          window_device.Destroy();
          cpu_device.Destroy();
     }

}