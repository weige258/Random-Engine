#pragma once
#include "Platform/CPU/CPUInfo.hpp"

namespace RandEngine::Systems::DeviceSystems::CPUSystems{
    class  CPUSystem
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