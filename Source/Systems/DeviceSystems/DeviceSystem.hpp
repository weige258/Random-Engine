#pragma once
#include "Systems/DeviceSystems/OSSystems/OSSystem.hpp"
#include "Systems/DeviceSystems/WindowSystems/WindowSystem.hpp"
#include "Systems/DeviceSystems/CPUSystems/CPUSystem.hpp"
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems::DeviceSystems
{
       
    class DeviceSystem:Systems::ISystem
    {
        DeviceSystems::OSSystems::OSSystem os_system;
        DeviceSystems::CPUSystems::CPUSystem cpu_system;
        DeviceSystems::WindowSystems::WindowSystem window_system;

        public:
            void Init();

            void Run(System &system);

            void Destroy();
    } ;
}