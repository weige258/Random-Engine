#pragma once
#include "Systems/DeviceSystems/OSDivices/OSDivice.hpp"
#include "Systems/DeviceSystems/WindowDevices/WindowDevice.hpp"

namespace RandEngine::Core::Systems::DeviceSystems
{
       
    class DeviceSystem
    {
        DeviceSystems::OSDevices::OSDevice os_device;
        DeviceSystems::WindowDevices::WindowDevice window_device;

        public:
            void Init();

            void Run();

            void Destroy();
    } ;
}