#include "OSInfo.hpp"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <sys/utsname.h>
#endif

namespace RandEngine::Platform::OS
{
    std::string_view GetTypeName(OSType type) noexcept
    {
        switch (type)
        {
            case OSType::Windows:     return "Windows";
            case OSType::Linux:       return "Linux";
            case OSType::MacOS:       return "macOS";
            case OSType::Android:     return "Android";
            case OSType::iOS:         return "iOS";
            case OSType::WebAssembly: return "WebAssembly";
            default:                  return "Unknown";
        }
    }

    std::string_view GetEnvironmentName(OSTargetEnvironment environment) noexcept
    {
        switch (environment)
        {
            case OSTargetEnvironment::Desktop: return "Desktop";
            case OSTargetEnvironment::Mobile:  return "Mobile";
            case OSTargetEnvironment::Console: return "Console";
            case OSTargetEnvironment::Web:     return "Web";
            default:                           return "Unknown";
        }
    }

    OSInfo DetectOSInfo()
    {
        OSInfo info{};

#if defined(_WIN32)
        info.type = OSType::Windows;
        info.environment = OSTargetEnvironment::Desktop;

        using pfnRtlGetVersion = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
        if (HMODULE hNtdll = GetModuleHandleA("ntdll.dll"))
        {
            auto RtlGetVersion = reinterpret_cast<pfnRtlGetVersion>(GetProcAddress(hNtdll, "RtlGetVersion"));
            if (RtlGetVersion)
            {
                RTL_OSVERSIONINFOW rovi = { sizeof(rovi) };
                if (RtlGetVersion(&rovi) == 0)
                {
                    info.version.major = rovi.dwMajorVersion;
                    info.version.minor = rovi.dwMinorVersion;
                    info.version.build = rovi.dwBuildNumber;
                    info.version.display_name = "Windows " + std::to_string(rovi.dwMajorVersion) +
                                                " (Build " + std::to_string(rovi.dwBuildNumber) + ")";
                }
            }
        }
#else
        struct utsname uname_data;
        if (uname(&uname_data) == 0)
        {
            info.version.display_name = std::string(uname_data.sysname) + " " + uname_data.release;
        }

#if defined(__ANDROID__)
        info.type = OSType::Android;
        info.environment = OSTargetEnvironment::Mobile;
#elif defined(__linux__)
        info.type = OSType::Linux;
        info.environment = OSTargetEnvironment::Desktop;
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        info.type = OSType::iOS;
        info.environment = OSTargetEnvironment::Mobile;
    #else
        info.type = OSType::MacOS;
        info.environment = OSTargetEnvironment::Desktop;
    #endif
#elif defined(__EMSCRIPTEN__)
        info.type = OSType::WebAssembly;
        info.environment = OSTargetEnvironment::Web;
#endif
#endif

        return info;
    }
}