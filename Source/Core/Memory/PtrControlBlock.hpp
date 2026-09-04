#pragma once
#include <atomic>
#include <cstdint>

namespace RandomEngine::Core::Memory
{
    struct PtrControlBlock
    {
        std::atomic<void*> target_ptr{nullptr}; //[cite: 42]
        std::atomic<uint32_t> observer_count{0}; //[cite: 42]
        std::atomic<uint32_t> active_readers{0}; // 轻量级并发读取计数器[cite: 42]
    };
}