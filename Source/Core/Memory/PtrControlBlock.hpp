#pragma once
#include <atomic>
#include <cstdint>

namespace RandEngine::Core::Memory
{
    template <typename T>
    struct PtrControlBlock
    {
        std::atomic<T*> target_ptr{nullptr};
        std::atomic<uint32_t> observer_count{0};
        std::atomic<uint32_t> active_readers{0}; // 轻量级并发读取计数器
    };
}