#include "CPUDivice.hpp"

namespace RandEngine::Core::Systems::DeviceSystems::CPUDivices{
    void CPUDevice::Init(){
        cpu_info = {};
        cpu_info.DetectAll();
    }

    void CPUDevice::Run(){
        cpu_info.DetectRuntime();
    }

    void CPUDevice::Destroy(){
        cpu_info = {};
    }

    Platform::CPU::CPUInfo CPUDevice::GetCPUInfo(){
        return cpu_info;
    }
}