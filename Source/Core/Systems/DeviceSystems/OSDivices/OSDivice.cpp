#include "OSDivice.hpp"

namespace RandEngine::Core::Systems::DeviceSystems::OSDevices {

    std::string_view OSDevice::GetTypeName() const noexcept {
        return Platform::OS::GetTypeName(info.type);
    }

    std::string_view OSDevice::GetEnvironmentName() const noexcept {
        return Platform::OS::GetEnvironmentName(info.environment);
    }

    void OSDevice::Init() {
        info = Platform::OS::DetectOSInfo();
    }

}