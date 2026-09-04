#pragma once
#include "Platform/OS/OSInfo.hpp"

namespace RandomEngine::Systems::DeviceSystems
{
    class OSSystem
    {
    private:
        Platform::OS::OSInfo info{};

    public:
        void Init();

        [[nodiscard]] Platform::OS::OSType GetType() const noexcept { return info.type; }
        [[nodiscard]] Platform::OS::OSTargetEnvironment GetEnvironment() const noexcept { return info.environment; }
        [[nodiscard]] const Platform::OS::OSVersion &GetVersion() const noexcept { return info.version; }

        [[nodiscard]] std::string_view GetTypeName() const noexcept;
        [[nodiscard]] std::string_view GetEnvironmentName() const noexcept;
    };
}