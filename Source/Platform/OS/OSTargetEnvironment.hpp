#pragma once
#include <cstdint>

namespace RandomEngine::Platform::OS{
    enum class OSTargetEnvironment : uint8_t {
        Desktop,
        Mobile,
        Console,
        Web,
        Unknown
    };
}