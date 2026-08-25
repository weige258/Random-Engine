#include "CPUSystem.hpp"

namespace RandEngine::Systems::DeviceSystems{
    void CPUSystem::Init(){
        cpu_info = {};
        cpu_info.DetectAll();
    }

    void CPUSystem::Run(){
        cpu_info.DetectRuntime();
    }

    void CPUSystem::Destroy(){
        cpu_info = {};
    }

    Platform::CPU::CPUInfo CPUSystem::GetCPUInfo(){
        return cpu_info;
    }
}