#pragma once

#include "PtrControlBlock.hpp"
#include <atomic>
#include <concepts>
#include <utility>
#include <functional>

namespace RandomEngine::Core::Memory
{
    template <typename T>
    struct MasterPtr;

    template <typename T>
    class ObserverPtr
    {
    private:
        T *ptr = nullptr;
        PtrControlBlock *block = nullptr;

        void AddRef() noexcept
        {
            if (block)
                block->AddOwnerRef();
        }

        void Release() noexcept
        {
            PtrControlBlock *old = block;

            ptr = nullptr;
            block = nullptr;

            if (old)
                old->ReleaseOwnerRef();
        }

        void Swap(ObserverPtr &other) noexcept
        {
            std::swap(ptr, other.ptr);
            std::swap(block, other.block);
        }

        template <typename U>
        friend class ObserverPtr;

        template <typename Target, typename Source>
        friend ObserverPtr<Target> dynamic_observer_cast(
            const ObserverPtr<Source> &) noexcept;

        template <typename Target, typename Source>
        friend ObserverPtr<Target> static_observer_cast(
            const ObserverPtr<Source> &) noexcept;

    public:
        class ScopedRef
        {
        private:
            T *ptr = nullptr;
            PtrControlBlock *block = nullptr;

            friend class ObserverPtr;

            ScopedRef(T *p, PtrControlBlock *b) noexcept
                : ptr(p), block(b)
            {
            }

            void Release() noexcept
            {
                if (block)
                {
                    block->Leave();
                    ptr = nullptr;
                    block = nullptr;
                }
            }

        public:
            ScopedRef() noexcept = default;

            ~ScopedRef()
            {
                Release();
            }

            ScopedRef(const ScopedRef &) = delete;
            ScopedRef &operator=(const ScopedRef &) = delete;

            ScopedRef(ScopedRef &&other) noexcept
                : ptr(std::exchange(other.ptr, nullptr)),
                  block(std::exchange(other.block, nullptr))
            {
            }

            ScopedRef &operator=(ScopedRef &&other) noexcept
            {
                if (this != &other)
                {
                    Release();

                    ptr = std::exchange(other.ptr, nullptr);
                    block = std::exchange(other.block, nullptr);
                }

                return *this;
            }

            [[nodiscard]] T *Get() const noexcept { return ptr; }
            [[nodiscard]] T *operator->() const noexcept { return ptr; }
            [[nodiscard]] T &operator*() const noexcept { return *ptr; }

            explicit operator bool() const noexcept
            {
                return ptr != nullptr;
            }
        };

        ObserverPtr() noexcept = default;

        ObserverPtr(std::nullptr_t) noexcept
            : ptr(nullptr), block(nullptr)
        {
        }

        explicit ObserverPtr(const MasterPtr<T> &master) noexcept
            : ptr(master.Get()), block(master.block)
        {
            AddRef();
        }

        template <typename U>
            requires std::convertible_to<U *, T *>
        explicit ObserverPtr(const MasterPtr<U> &master) noexcept
            : ptr(master.Get()), block(master.block)
        {
            AddRef();
        }

        template <typename U>
            requires std::convertible_to<U *, T *>
        ObserverPtr(const ObserverPtr<U> &other) noexcept
            : ptr(other.ptr), block(other.block)
        {
            AddRef();
        }

        ObserverPtr(const ObserverPtr &other) noexcept
            : ptr(other.ptr), block(other.block)
        {
            AddRef();
        }

        ObserverPtr(ObserverPtr &&other) noexcept
            : ptr(std::exchange(other.ptr, nullptr)),
              block(std::exchange(other.block, nullptr))
        {
        }

        ~ObserverPtr()
        {
            Release();
        }

        ObserverPtr &operator=(const ObserverPtr &other) noexcept
        {
            if (this != &other)
            {
                ObserverPtr temp(other);
                Swap(temp);
            }

            return *this;
        }

        ObserverPtr &operator=(ObserverPtr &&other) noexcept
        {
            if (this != &other)
            {
                Release();

                ptr = std::exchange(other.ptr, nullptr);
                block = std::exchange(other.block, nullptr);
            }

            return *this;
        }

        ObserverPtr &operator=(std::nullptr_t) noexcept
        {
            Release();
            return *this;
        }

        // 真正的生命周期保护入口。
        [[nodiscard]] ScopedRef Lock() const noexcept
        {
            if (!block || !ptr)
                return {};

            if (!block->TryEnter())
                return {};

            return ScopedRef{ptr, block};
        }

        [[nodiscard]] bool Expired() const noexcept
        {
            return !block ||
                block->target_ptr.load(
                    std::memory_order_acquire) == nullptr;
        }

        // 只返回裸指针，不提供生命周期保护。
        // 并发删除场景不要在 Get() 后直接解引用。
        [[nodiscard]] T *Get() const noexcept
        {
            return (
                block &&
                block->target_ptr.load(
                    std::memory_order_acquire) != nullptr)
                ? ptr
                : nullptr;
        }

        [[nodiscard]] T *operator->() const noexcept { return Get(); }
        [[nodiscard]] T &operator*() const noexcept { return *Get(); }

        explicit operator bool() const noexcept
        {
            return Get() != nullptr;
        }

        // 控制块身份，用于哈希与一致性比较，不随对象过期而变化。
        [[nodiscard]] const PtrControlBlock *GetBlockId() const noexcept
        {
            return block;
        }

        // 跨模板类型比较运算符支持
        template <typename U>
        bool operator==(const ObserverPtr<U> &other) const noexcept { return block == other.block; }
        template <typename U>
        bool operator!=(const ObserverPtr<U> &other) const noexcept { return block != other.block; }

        bool operator==(std::nullptr_t) const noexcept { return Get() == nullptr; }
        bool operator!=(std::nullptr_t) const noexcept { return Get() != nullptr; }
    };

    // 动态向下转型工具：类似于 std::dynamic_pointer_cast
    template <typename Target, typename Source>
    ObserverPtr<Target> dynamic_observer_cast(
        const ObserverPtr<Source> &src) noexcept
    {
        auto lease = src.Lock();

        if (!lease)
            return {};

        Target *target = dynamic_cast<Target *>(lease.Get());

        if (!target)
            return {};

        ObserverPtr<Target> result;
        result.ptr = target;
        result.block = src.block;
        result.AddRef();

        return result;
    }

    // 静态转型工具：类似于 std::static_pointer_cast
    template <typename Target, typename Source>
    ObserverPtr<Target> static_observer_cast(
        const ObserverPtr<Source> &src) noexcept
    {
        auto lease = src.Lock();

        if (!lease)
            return {};

        Target *target = static_cast<Target *>(lease.Get());

        ObserverPtr<Target> result;
        result.ptr = target;
        result.block = src.block;
        result.AddRef();

        return result;
    }
}

// 哈希支持
template <typename T>
struct std::hash<RandomEngine::Core::Memory::ObserverPtr<T>>
{
    size_t operator()(const RandomEngine::Core::Memory::ObserverPtr<T> &p) const noexcept
    {
        return std::hash<const RandomEngine::Core::Memory::PtrControlBlock *>{}(p.GetBlockId());
    }
};