#pragma once

#include "PtrControlBlock.hpp"
#include <atomic>
#include <concepts>
#include <functional>

namespace RandomEngine::Core::Memory
{
    template <typename T>
    struct MasterPtr; //[cite: 41]

    template <typename T>
    class ObserverPtr
    {
    private:
        T *ptr = nullptr;
        PtrControlBlock *block = nullptr; //[cite: 41, 42]

        template <typename U>
        friend class ObserverPtr;

    public:
        class ScopedRef //[cite: 41]
        {
        private:
            T *ptr = nullptr;                 //[cite: 41]
            PtrControlBlock *block = nullptr; //[cite: 41, 42]

            friend class ObserverPtr;
            ScopedRef(T *p, PtrControlBlock *b) noexcept : ptr(p), block(b) {} //[cite: 41]

        public:
            ScopedRef() noexcept = default; //[cite: 41]
            ~ScopedRef()                    //[cite: 41]
            {
                if (block) //[cite: 41]
                {
                    block->active_readers.fetch_sub(1, std::memory_order_release); //[cite: 41]
                }
            }

            ScopedRef(const ScopedRef &) = delete;            //[cite: 41]
            ScopedRef &operator=(const ScopedRef &) = delete; //[cite: 41]

            ScopedRef(ScopedRef &&other) noexcept : ptr(other.ptr), block(other.block) //[cite: 41]
            {
                other.ptr = nullptr;   //[cite: 41]
                other.block = nullptr; //[cite: 41]
            }

            ScopedRef &operator=(ScopedRef &&other) noexcept //[cite: 41]
            {
                if (this != &other) //[cite: 41]
                {
                    if (block)                                                         //[cite: 41]
                        block->active_readers.fetch_sub(1, std::memory_order_release); //[cite: 41]
                    ptr = other.ptr;                                                   //[cite: 41]
                    block = other.block;                                               //[cite: 41]
                    other.ptr = nullptr;                                               //[cite: 41]
                    other.block = nullptr;                                             //[cite: 41]
                }
                return *this; //[cite: 41]
            }

            [[nodiscard]] T *Get() const noexcept { return ptr; }              //[cite: 41]
            [[nodiscard]] T *operator->() const noexcept { return ptr; }       //[cite: 41]
            [[nodiscard]] T &operator*() const noexcept { return *ptr; }       //[cite: 41]
            explicit operator bool() const noexcept { return ptr != nullptr; } //[cite: 41]
        };

        ObserverPtr() noexcept = default; //[cite: 41]

        ObserverPtr(std::nullptr_t) noexcept : ptr(nullptr), block(nullptr) {}

        explicit ObserverPtr(const MasterPtr<T> &master) noexcept
            : ptr(master.Get()), block(master.block)
        {
            if (block)
                block->observer_count.fetch_add(1, std::memory_order_relaxed);
        }

        template <typename U>
            requires std::convertible_to<U *, T *>
        explicit ObserverPtr(const MasterPtr<U> &master) noexcept
            : ptr(master.Get()), block(master.block)
        {
            if (block)
                block->observer_count.fetch_add(1, std::memory_order_relaxed);
        }

        template <typename U>
            requires std::convertible_to<U *, T *>
        ObserverPtr(const ObserverPtr<U> &other) noexcept
            : ptr(other.ptr), block(other.block)
        {
            if (block)
                block->observer_count.fetch_add(1, std::memory_order_relaxed);
        }

        ~ObserverPtr() //[cite: 41]
        {
            if (block && block->observer_count.fetch_sub(1, std::memory_order_acq_rel) == 1) //[cite: 41]
            {
                if (block->target_ptr.load(std::memory_order_acquire) == nullptr) //[cite: 41]
                {
                    delete block; //[cite: 41]
                }
            }
        }

        ObserverPtr(const ObserverPtr &other) noexcept : ptr(other.ptr), block(other.block)
        {
            if (block)
                block->observer_count.fetch_add(1, std::memory_order_relaxed);
        }

        ObserverPtr &operator=(const ObserverPtr &other) noexcept
        {
            if (this != &other)
            {
                this->~ObserverPtr();
                ptr = other.ptr;     // <-- 补齐 ptr 拷贝[cite: 48]
                block = other.block; //[cite: 48]
                if (block)
                    block->observer_count.fetch_add(1, std::memory_order_relaxed); //[cite: 48]
            }
            return *this;
        }

        ObserverPtr(ObserverPtr &&other) noexcept : ptr(other.ptr), block(other.block)
        {
            other.ptr = nullptr; // 必须清空源 ptr
            other.block = nullptr;
        }

        ObserverPtr &operator=(ObserverPtr &&other) noexcept
        {
            if (this != &other)
            {
                this->~ObserverPtr();
                ptr = other.ptr;     // <-- 补齐 ptr 移动[cite: 48]
                block = other.block; //[cite: 48]
                other.ptr = nullptr;
                other.block = nullptr; //[cite: 48]
            }
            return *this;
        }

        ObserverPtr &operator=(std::nullptr_t) noexcept
        {
            if (block && block->observer_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                if (block->target_ptr.load(std::memory_order_acquire) == nullptr)
                {
                    delete block;
                }
            }
            ptr = nullptr;
            block = nullptr;
            return *this;
        }

        [[nodiscard]] ScopedRef Lock() const noexcept
        {
            if (!block)
                return {};

            block->active_readers.fetch_add(1, std::memory_order_relaxed);

            void *p_raw = block->target_ptr.load(std::memory_order_acquire);
            if (!p_raw)
            {
                block->active_readers.fetch_sub(1, std::memory_order_relaxed);
                return {};
            }

            return ScopedRef{ptr, block};
        }

        [[nodiscard]] bool Expired() const noexcept //[cite: 41]
        {
            return !block || block->target_ptr.load(std::memory_order_relaxed) == nullptr; //[cite: 41, 42]
        }

        [[nodiscard]] T *Get() const noexcept
        {
            return (block && block->target_ptr.load(std::memory_order_relaxed)) ? ptr : nullptr;
        }

        [[nodiscard]] T *operator->() const noexcept { return Get(); }       //[cite: 41]
        [[nodiscard]] T &operator*() const noexcept { return *Get(); }       //[cite: 41]
        explicit operator bool() const noexcept { return Get() != nullptr; } //[cite: 41]

        // 跨模板类型比较运算符支持[cite: 41]
        template <typename U>
        bool operator==(const ObserverPtr<U> &other) const noexcept { return block == other.block; } //[cite: 41]
        template <typename U>
        bool operator!=(const ObserverPtr<U> &other) const noexcept { return block != other.block; } //[cite: 41]

        bool operator==(std::nullptr_t) const noexcept { return Get() == nullptr; } //[cite: 41]
        bool operator!=(std::nullptr_t) const noexcept { return Get() != nullptr; } //[cite: 41]

        friend struct MasterPtr<T>; //[cite: 41]

        template <typename Target, typename Source>
        friend ObserverPtr<Target> dynamic_observer_cast(const ObserverPtr<Source> &src) noexcept;
        template <typename Target, typename Source>
        friend ObserverPtr<Target> static_observer_cast(const ObserverPtr<Source> &src) noexcept;
    };

    // 动态向下转型工具：类似于 std::dynamic_pointer_cast[cite: 41]
    template <typename Target, typename Source>
    ObserverPtr<Target> dynamic_observer_cast(const ObserverPtr<Source> &src) noexcept
    {
        Target *target_ptr = dynamic_cast<Target *>(src.Get());
        if (!src || !target_ptr)
            return {};

        ObserverPtr<Target> dst;
        dst.ptr = target_ptr; // 保存计算偏移后的正确接口指针
        dst.block = src.block;
        if (dst.block)
        {
            dst.block->observer_count.fetch_add(1, std::memory_order_relaxed);
        }
        return dst;
    }

    // 静态转型工具：类似于 std::static_pointer_cast[cite: 41]
    template <typename Target, typename Source>
    ObserverPtr<Target> static_observer_cast(const ObserverPtr<Source> &src) noexcept
    {
        if (!src)
            return {};

        ObserverPtr<Target> dst;
        dst.block = src.block;
        if (dst.block)
        {
            dst.block->observer_count.fetch_add(1, std::memory_order_relaxed);
        }
        return dst;
    }
}

// 哈希支持[cite: 41]
template <typename T>
struct std::hash<RandomEngine::Core::Memory::ObserverPtr<T>>
{
    size_t operator()(const RandomEngine::Core::Memory::ObserverPtr<T> &p) const noexcept
    {
        return std::hash<T *>{}(p.Get());
    }
};