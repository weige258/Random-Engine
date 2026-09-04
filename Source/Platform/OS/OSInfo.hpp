#pragma once
#include "OSType.hpp"
#include "OSVersion.hpp"
#include "OSTargetEnvironment.hpp"

namespace RandomEngine::Platform::OS
{
    struct OSInfo
    {
        OSType type = OSType::Unknown;
        OSTargetEnvironment environment = OSTargetEnvironment::Unknown;
        OSVersion version{};
    };

    OSInfo DetectOSInfo();

    std::string_view GetTypeName(OSType type) noexcept;
    std::string_view GetEnvironmentName(OSTargetEnvironment environment) noexcept;
}