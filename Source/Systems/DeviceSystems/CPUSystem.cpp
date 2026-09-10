#include "CPUSystem.hpp"

namespace RandomEngine::Systems::DeviceSystems{

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

    const Platform::CPU::CPUInfo& CPUSystem::GetCPUInfo() const{
        return cpu_info;
    }

    Platform::CPU::CPUInfo& CPUSystem::GetCPUInfo(){
        return cpu_info;
    }
}