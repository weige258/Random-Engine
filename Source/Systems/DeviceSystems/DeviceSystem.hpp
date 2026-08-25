#pragma once
#include "Systems/DeviceSystems/OSSystem.hpp"
#include "Systems/DeviceSystems/WindowSystem.hpp"
#include "Systems/DeviceSystems/CPUSystem.hpp"
#include "Systems/ISystem.hpp"

namespace RandEngine::Systems::DeviceSystems
{

    class DeviceSystem : Systems::ISystem
    {
    public:
        DeviceSystems::OSSystem os_system;
        DeviceSystems::CPUSystem cpu_system;
        DeviceSystems::WindowSystem window_system;

        void Init(System &system);

        void Run(System &system);

        void Destroy();
    };
}