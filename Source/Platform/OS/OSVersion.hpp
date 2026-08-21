#pragma once
#include <cstdint>
#include <string>


namespace RandEngine::Platform::OS {
    struct OSVersion {
        uint32_t major = 0;
        uint32_t minor = 0;
        uint32_t build = 0;
        std::string display_name; // 例: "Windows 11 Build 22631"
    };
}