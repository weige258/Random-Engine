#pragma once
#include "Platform/CPU/CPUInfo.hpp"

namespace RandEngine::Core::Systems::DeviceSystems::CPUDivices{
    class  CPUDevice
    {
        private:
        Platform::CPU::CPUInfo cpu_info;
  
        public:
        void Init();

        void Run();

        void Destroy();

         Platform::CPU::CPUInfo GetCPUInfo();
    };
}