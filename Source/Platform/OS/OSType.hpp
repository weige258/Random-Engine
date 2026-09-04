#pragma once
#include <cstdint>

namespace RandomEngine::Platform::OS{
    enum class OSType : uint8_t {
        Windows,
        Linux,
        MacOS,
        Android,
        iOS,
        WebAssembly,
        Unknown
    };
}