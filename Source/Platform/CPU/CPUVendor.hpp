#pragma once
#include <stdint.h>

namespace RandomEngine::Platform::CPU{

enum class CPUVendor : uint8_t
    {
        Unknown,
        Intel,
        AMD,
        Apple,
        Qualcomm,
        ARM
    };

}