#pragma once
#include <stdint.h>

namespace RandEngine::Platform::CPU {
    enum class CPUArchitecture : uint8_t
    {
        Unknown,
        x86,
        x64,
        ARM32,
        ARM64,
        RISCV
    }; 
}