#pragma once
#include <cstdint>

namespace RandEngine::Platform::OS{
    enum class OSTargetEnvironment : uint8_t {
        Desktop,
        Mobile,
        Console,
        Web,
        Unknown
    };
}