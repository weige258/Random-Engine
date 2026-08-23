#pragma once

#include "PtrControlBlock.hpp"
#include <atomic>
#include <concepts>

namespace RandEngine::Core::Memory
{
    template <typename T>
    struct MasterPtr;

    template <typename T>
    class ObserverPtr
    {
    private:
        PtrControlBlock<T> *block = nullptr;

    public:
        // RAII 保护句柄：极轻量，不进行 shared_ptr 级别的强引用升级
        class ScopedRef
        {
        private:
            T *ptr = nullptr;
            PtrControlBlock<T> *block = nullptr;

            friend class ObserverPtr;
            ScopedRef(T *p, PtrControlBlock<T> *b) noexcept : ptr(p), block(b) {}

        public:
            ScopedRef() noexcept = default;
            ~ScopedRef()
            {
                if (block)
                {
                    // 释放读取权：仅需 release 内存顺序
                    block->active_readers.fetch_sub(1, std::memory_order_release);
                }
            }

            ScopedRef(const ScopedRef &) = delete;
            ScopedRef &operator=(const ScopedRef &) = delete;

            ScopedRef(ScopedRef &&other) noexcept : ptr(other.ptr), block(other.block)
            {
                other.ptr = nullptr;
                other.block = nullptr;
            }

            ScopedRef &operator=(ScopedRef &&other) noexcept
            {
                if (this != &other)
                {
                    if (block)
                        block->active_readers.fetch_sub(1, std::memory_order_release);
                    ptr = other.ptr;
                    block = other.block;
                    other.ptr = nullptr;
                    other.block = nullptr;
                }
                return *this;
            }

            [[nodiscard]] T *Get() const noexcept
            {
                return ptr;
            }

            [[nodiscard]] T *operator->() const noexcept
            {
                return ptr;
            }

            [[nodiscard]] T &operator*() const noexcept
            {
                return *ptr;
            }
            explicit operator bool() const noexcept
            {
                return ptr != nullptr;
            }
        };

        ObserverPtr() noexcept = default;

        explicit ObserverPtr(const MasterPtr<T> &master) noexcept : block(master.block)
        {
            if (block)
            {
                block->observer_count.fetch_add(1, std::memory_order_relaxed);
            }
        }

        template <typename U>
            requires std::convertible_to<U *, T *>
        ObserverPtr(const ObserverPtr<U> &other) noexcept : block(other.block)
        {
            if (block)
            {
                block->observer_count.fetch_add(1, std::memory_order_relaxed);
            }
        }

        ~ObserverPtr()
        {
            if (block && block->observer_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                // 若此时 target_ptr 已空且无观察者，销毁控制块
                if (block->target_ptr.load(std::memory_order_acquire) == nullptr)
                {
                    delete block;
                }
            }
        }

        ObserverPtr(const ObserverPtr &other) noexcept : block(other.block)
        {
            if (block)
                block->observer_count.fetch_add(1, std::memory_order_relaxed);
        }

        ObserverPtr &operator=(const ObserverPtr &other) noexcept
        {
            if (this != &other)
            {
                this->~ObserverPtr();
                block = other.block;
                if (block)
                    block->observer_count.fetch_add(1, std::memory_order_relaxed);
            }
            return *this;
        }

        ObserverPtr(ObserverPtr &&other) noexcept : block(other.block)
        {
            other.block = nullptr;
        }

        ObserverPtr &operator=(ObserverPtr &&other) noexcept
        {
            if (this != &other)
            {
                this->~ObserverPtr();
                block = other.block;
                other.block = nullptr;
            }
            return *this;
        }

        // 核心高性能 Lock 函数：相比 weak_ptr::lock() 避免了昂贵的强引用控制块构造
        [[nodiscard]] ScopedRef Lock() const noexcept
        {
            if (!block)
                return {};

            // 1. 使用 relaxed 预占读取位置，极小化指令开销
            block->active_readers.fetch_add(1, std::memory_order_relaxed);

            // 2. 使用 acquire 同步 target_ptr 状态
            T *p = block->target_ptr.load(std::memory_order_acquire);
            if (!p)
            {
                block->active_readers.fetch_sub(1, std::memory_order_relaxed);
                return {};
            }

            return ScopedRef{p, block};
        }
        
        [[nodiscard]] bool Expired() const noexcept
        {
            return !block || block->target_ptr.load(std::memory_order_relaxed) == nullptr;
        }

        //访问
        [[nodiscard]] T *Get() const noexcept
        {
            return block ? block->target_ptr.load(std::memory_order_relaxed) : nullptr; //[cite: 2, 3]
        }

        [[nodiscard]] T *operator->() const noexcept
        {
            return Get();
        }

        [[nodiscard]] T &operator*() const noexcept
        {
            return *Get();
        }

        explicit operator bool() const noexcept
        {
            return Get() != nullptr;
        }

        friend struct MasterPtr<T>;
    };
}