#pragma once
#include "Systems/DeviceSystems/OSDivices/OSDivice.hpp"
#include "Systems/DeviceSystems/WindowDevices/WindowDevice.hpp"
#include "Systems/DeviceSystems/CPUDivices/CPUDivice.hpp"

namespace RandEngine::Systems::DeviceSystems
{
       
    class DeviceSystem
    {
        DeviceSystems::OSDevices::OSDevice os_device;
        DeviceSystems::CPUDivices::CPUDevice cpu_device;
        DeviceSystems::WindowDevices::WindowDevice window_device;

        public:
            void Init();

            void Run();

            void Destroy();
    } ;
}