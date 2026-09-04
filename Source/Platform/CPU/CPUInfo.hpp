#pragma once
#include <stdint.h>
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

        std::string vendor_id;    // 原生识别符 (如 "GenuineIntel")
        std::string brand_string; // CPU 型号名称 (如 "13th Gen Intel(R) Core(TM) i9-13900K")

        uint32_t logical_processor_count = 0;
        uint32_t physical_core_count = 0;

        uint32_t cache_line_size = 64;
        uint32_t l1_cache_size = 0;
        uint32_t l2_cache_size = 0;
        uint32_t l3_cache_size = 0;

        // 指令集特性
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
        std::vector<float> core_usage_percentages;  

        void DetectAll();

        void DetectRuntime();

        void DetectArchitectureAndVendor();
        void DetectBrandString();
        void DetectCoreCounts();
        void DetectInstructionSets();
        void DetectCacheInfo();
    };
}