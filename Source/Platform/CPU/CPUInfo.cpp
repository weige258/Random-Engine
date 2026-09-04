#include "CPUInfo.hpp"

#if defined(_WIN32)
    #include <windows.h>
#endif

// 跨编译器 Intrinsics 适配
#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
    #if defined(__i386__) || defined(__x86_64__)
        #include <cpuid.h>
        #include <x86intrin.h>
    #endif
#endif

namespace RandomEngine::Platform::CPU
{
    namespace
    {
#if defined(_WIN32)
        uint64_t FileTimeToUInt64(const FILETIME& ft)
        {
            return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        }
#endif

        // 兼容 MSVC / GCC / Clang 的 CPUID 封装
        inline void NativeCPUID(int regs[4], int leaf, int subleaf = 0)
        {
#if defined(_MSC_VER)
            __cpuidex(regs, leaf, subleaf);
#elif defined(__GNUC__) || defined(__clang__)
            __cpuid_count(leaf, subleaf, regs[0], regs[1], regs[2], regs[3]);
#else
            regs[0] = regs[1] = regs[2] = regs[3] = 0;
#endif
        }

        // 兼容 MSVC / GCC / Clang 的 XGETBV 封装
        inline uint64_t NativeXGetBV(uint32_t xcr)
        {
#if defined(_MSC_VER)
            return _xgetbv(xcr);
#elif defined(__GNUC__) || defined(__clang__)
            uint32_t eax, edx;
            __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
            return (static_cast<uint64_t>(edx) << 32) | eax;
#else
            return 0;
#endif
        }
    }

    // ===================================================================
    // 公共 API 实现
    // ===================================================================

    void CPUInfo::DetectAll()
    {
        DetectArchitectureAndVendor();
        DetectBrandString();
        DetectCoreCounts();
        DetectInstructionSets();
        DetectCacheInfo();
    }

    void CPUInfo::DetectRuntime()
    {
#if defined(_WIN32)
        static uint64_t prev_idle_time   = 0;
        static uint64_t prev_kernel_time = 0;
        static uint64_t prev_user_time   = 0;

        FILETIME idle_time, kernel_time, user_time;
        if (GetSystemTimes(&idle_time, &kernel_time, &user_time))
        {
            uint64_t idle   = FileTimeToUInt64(idle_time);
            uint64_t kernel = FileTimeToUInt64(kernel_time);
            uint64_t user   = FileTimeToUInt64(user_time);

            uint64_t delta_idle   = idle - prev_idle_time;
            uint64_t delta_kernel = kernel - prev_kernel_time;
            uint64_t delta_user   = user - prev_user_time;

            uint64_t delta_total = delta_kernel + delta_user;

            if (delta_total > 0)
            {
                float idle_fraction = static_cast<float>(delta_idle) / static_cast<float>(delta_total);
                total_usage_percentage = (1.0f - idle_fraction) * 100.0f;

                if (total_usage_percentage < 0.0f) total_usage_percentage = 0.0f;
                if (total_usage_percentage > 100.0f) total_usage_percentage = 100.0f;
            }

            prev_idle_time   = idle;
            prev_kernel_time = kernel;
            prev_user_time   = user;
        }
#endif
    }

    // ===================================================================
    // 私有子检测模块实现
    // ===================================================================

    void CPUInfo::DetectArchitectureAndVendor()
    {
#if defined(_M_X64) || defined(__x86_64__)
        architecture = CPUArchitecture::x64;
#elif defined(_M_IX86) || defined(__i386__)
        architecture = CPUArchitecture::x86;
#elif defined(_M_ARM64) || defined(__aarch64__)
        architecture = CPUArchitecture::ARM64;
#elif defined(_M_ARM) || defined(__arm__)
        architecture = CPUArchitecture::ARM32;
#else
        architecture = CPUArchitecture::Unknown;
#endif

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
        int regs[4] = {0};
        NativeCPUID(regs, 0);

        char vendor_buf[13] = {0};
        *reinterpret_cast<int*>(vendor_buf)     = regs[1]; // EBX
        *reinterpret_cast<int*>(vendor_buf + 4) = regs[3]; // EDX
        *reinterpret_cast<int*>(vendor_buf + 8) = regs[2]; // ECX
        vendor_id = vendor_buf;

        if (vendor_id == "GenuineIntel") vendor = CPUVendor::Intel;
        else if (vendor_id == "AuthenticAMD") vendor = CPUVendor::AMD;
#elif defined(_M_ARM64) || defined(__aarch64__)
        vendor = CPUVendor::ARM;
#endif
    }

    void CPUInfo::DetectBrandString()
    {
#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
        int regs[4] = {0};
        NativeCPUID(regs, static_cast<int>(0x80000000));

        if (static_cast<uint32_t>(regs[0]) >= 0x80000004)
        {
            char brand_buf[49] = {0};
            for (uint32_t i = 0; i < 3; ++i)
            {
                NativeCPUID(reinterpret_cast<int*>(brand_buf + i * 16), static_cast<int>(0x80000002 + i), 0);
            }
            brand_string = brand_buf;
        }
#endif
    }

    void CPUInfo::DetectCoreCounts()
    {
#if defined(_WIN32)
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        logical_processor_count = sys_info.dwNumberOfProcessors;

        DWORD return_len = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &return_len);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && return_len > 0)
        {
            std::vector<uint8_t> buffer(return_len);
            auto ptr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
            if (GetLogicalProcessorInformationEx(RelationProcessorCore, ptr, &return_len))
            {
                uint32_t cores = 0;
                DWORD offset = 0;
                while (offset < return_len)
                {
                    auto current = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
                    if (current->Relationship == RelationProcessorCore)
                    {
                        cores++;
                    }
                    offset += current->Size;
                }
                physical_core_count = cores;
            }
        }

        if (physical_core_count == 0)
        {
            physical_core_count = logical_processor_count;
        }
#endif
    }

    void CPUInfo::DetectInstructionSets()
    {
#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
        int regs[4] = {0};
        NativeCPUID(regs, 0);
        int max_ids = regs[0];

        bool os_supports_avx = false;
        bool os_supports_avx512 = false;

        if (max_ids >= 1)
        {
            NativeCPUID(regs, 1);
            has_sse3  = (regs[2] & (1 << 0)) != 0;
            has_sse41 = (regs[2] & (1 << 19)) != 0;
            has_sse42 = (regs[2] & (1 << 20)) != 0;
            has_sse2  = (regs[3] & (1 << 26)) != 0;

            bool cpu_has_fma     = (regs[2] & (1 << 12)) != 0;
            bool cpu_has_avx     = (regs[2] & (1 << 28)) != 0;
            bool cpu_has_osxsave = (regs[2] & (1 << 27)) != 0;

            if (cpu_has_osxsave)
            {
                uint64_t xcr0 = NativeXGetBV(0);
                os_supports_avx    = (xcr0 & 0x6) == 0x6;
                os_supports_avx512 = (xcr0 & 0xE6) == 0xE6;
            }

            has_avx = cpu_has_avx && os_supports_avx;
            has_fma = cpu_has_fma && os_supports_avx;
        }

        if (max_ids >= 7)
        {
            NativeCPUID(regs, 7, 0);
            has_avx2   = ((regs[1] & (1 << 5)) != 0) && has_avx;
            has_avx512 = ((regs[1] & (1 << 16)) != 0) && os_supports_avx512;
        }
#elif defined(_M_ARM64) || defined(__aarch64__)
        has_neon = true;
#endif
    }

    void CPUInfo::DetectCacheInfo()
    {
#if defined(_WIN32)
        DWORD return_len = 0;
        GetLogicalProcessorInformationEx(RelationCache, nullptr, &return_len);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && return_len > 0)
        {
            std::vector<uint8_t> buffer(return_len);
            auto ptr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
            if (GetLogicalProcessorInformationEx(RelationCache, ptr, &return_len))
            {
                DWORD offset = 0;
                while (offset < return_len)
                {
                    auto current = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
                    if (current->Relationship == RelationCache)
                    {
                        const auto& cache = current->Cache;
                        cache_line_size = cache.LineSize;

                        if (cache.Level == 1) l1_cache_size += cache.CacheSize / 1024;
                        else if (cache.Level == 2) l2_cache_size += cache.CacheSize / 1024;
                        else if (cache.Level == 3) l3_cache_size += cache.CacheSize / 1024;
                    }
                    offset += current->Size;
                }
            }
        }
#endif
    }
}