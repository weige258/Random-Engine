#include "OSSystem.hpp"

namespace RandomEngine::Systems::DeviceSystems {

    std::string_view OSSystem::GetTypeName() const noexcept {
        return Platform::OS::GetTypeName(info.type);
    }

    std::string_view OSSystem::GetEnvironmentName() const noexcept {
        return Platform::OS::GetEnvironmentName(info.environment);
    }

    void OSSystem::Init() {
        info = Platform::OS::DetectOSInfo();
    }

}