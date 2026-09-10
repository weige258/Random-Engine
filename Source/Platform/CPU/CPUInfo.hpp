#pragma once
#include <stdint.h>
#include <chrono>
#include "CPUArchitecture.hpp"
#include "CPUVendor.hpp"
#include <string>
#include <vector>

namespace RandomEngine::Platform::CPU
{

    struct CPUInfo
    {
        CPUVendor vendor = CPUVendor::Unknown;
        CPUArchitecture architecture = CPUArchitecture::Unknown;

        std::string vendor_id;
        std::string brand_string;

        uint32_t logical_processor_count = 0;
        uint32_t physical_core_count = 0;

        uint32_t cache_line_size = 64;
        uint32_t l1_cache_size = 0;
        uint32_t l2_cache_size = 0;
        uint32_t l3_cache_size = 0;

        bool has_sse2 = false;
        bool has_sse3 = false;
        bool has_sse41 = false;
        bool has_sse42 = false;
        bool has_avx = false;
        bool has_avx2 = false;
        bool has_fma = false;
        bool has_avx512 = false;
        bool has_neon = false;

        float total_usage_percentage = 0.0f;
        float system_usage_percentage = 0.0f;
        std::vector<float> core_usage_percentages;

        uint64_t m_window_start_proc = 0;
        uint64_t m_window_start_system_idle = 0;
        uint64_t m_window_start_system_total = 0;
        std::chrono::steady_clock::time_point m_window_start_wall{};
        bool m_runtime_initialized = false;

        void DetectAll();

        void DetectRuntime();

        void DetectArchitectureAndVendor();
        void DetectBrandString();
        void DetectCoreCounts();
        void DetectInstructionSets();
        void DetectCacheInfo();
    };
}