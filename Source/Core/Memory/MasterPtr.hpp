#pragma once

#include "PtrControlBlock.hpp"
#include "ObserverPtr.hpp"
#include <thread>
#include <utility>

namespace RandEngine::Core::Memory
{
    template <typename T>
    struct MasterPtr
    {
    private:
        T* ptr = nullptr;
        PtrControlBlock<T>* block = nullptr;

    public:
        explicit MasterPtr(T* resource = nullptr) : ptr(resource)
        {
            if (ptr)
            {
                block = new PtrControlBlock<T>();
                block->target_ptr.store(ptr, std::memory_order_relaxed);
                block->observer_count.store(1, std::memory_order_relaxed); // Master 自己算 1 个生命周期屏障
            }
        }

        ~MasterPtr()
        {
            Reset();
        }

        MasterPtr(const MasterPtr&) = delete;
        MasterPtr& operator=(const MasterPtr&) = delete;

        MasterPtr(MasterPtr&& other) noexcept : ptr(other.ptr), block(other.block)
        {
            other.ptr = nullptr;
            other.block = nullptr;
        }

        MasterPtr& operator=(MasterPtr&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                ptr = other.ptr;
                block = other.block;
                other.ptr = nullptr;
                other.block = nullptr;
            }
            return *this;
        }

        void Reset() noexcept
        {
            if (block)
            {
                // 1. release 标记置空，阻止后续新的 ObserverPtr::Lock() 成功
                block->target_ptr.store(nullptr, std::memory_order_release);

                // 2. 无锁等待：仅当恰好有其他线程在 Lock() 内的微秒级临界区时极短暂 Yield
                while (block->active_readers.load(std::memory_order_acquire) > 0)
                {
                    std::this_thread::yield();
                }

                // 3. 安全物理释放
                delete ptr;
                ptr = nullptr;

                // 4. 减去 Master 持有的计数
                if (block->observer_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    delete block;
                }
                block = nullptr;
            }
            else if (ptr)
            {
                delete ptr;
                ptr = nullptr;
            }
        }

        [[nodiscard]] T* Get() const noexcept { return ptr; }
        [[nodiscard]] T* operator->() const noexcept { return ptr; }
        [[nodiscard]] T& operator*() const noexcept { return *ptr; }
        explicit operator bool() const noexcept { return ptr != nullptr; }

        friend class ObserverPtr<T>;
    };
}