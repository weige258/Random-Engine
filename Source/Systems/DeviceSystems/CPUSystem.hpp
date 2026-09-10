#pragma once
#include "Platform/CPU/CPUInfo.hpp"

namespace RandomEngine::Systems::DeviceSystems{
    class  CPUSystem
    {
        private:
        Platform::CPU::CPUInfo cpu_info;
  
        public:
        void Init();

        void Run();

        void Destroy();

        const Platform::CPU::CPUInfo& GetCPUInfo() const;
        Platform::CPU::CPUInfo& GetCPUInfo();
    };
}