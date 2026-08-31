#pragma once

#include "PtrControlBlock.hpp"
#include "ObserverPtr.hpp"
#include <thread>
#include <utility>
#include <concepts>
#include <functional>

namespace RandEngine::Core::Memory
{
    template <typename T>
    struct MasterPtr
    {
    private:
        T *ptr = nullptr;                 //[cite: 40]
        PtrControlBlock *block = nullptr; // 配合类型擦除后的控制块[cite: 40, 42]

        template <typename U>
        friend struct MasterPtr;

        template <typename U>
        friend class ObserverPtr;

    public:
        MasterPtr(std::nullptr_t) noexcept : ptr(nullptr), block(nullptr) {}

        explicit MasterPtr(T *resource = nullptr) : ptr(resource) //[cite: 40]
        {
            if (ptr)
            {
                block = new PtrControlBlock();                                                //[cite: 40]
                block->target_ptr.store(static_cast<void *>(ptr), std::memory_order_relaxed); //[cite: 40, 42]
                block->observer_count.store(1, std::memory_order_relaxed);                    // Master 算 1 个生命周期屏障[cite: 40]
            }
        }

        ~MasterPtr()
        {
            Reset(); //[cite: 40]
        }

        MasterPtr(const MasterPtr &) = delete;            //[cite: 40]
        MasterPtr &operator=(const MasterPtr &) = delete; //[cite: 40]

        MasterPtr(MasterPtr &&other) noexcept : ptr(other.ptr), block(other.block) //[cite: 40]
        {
            other.ptr = nullptr;   //[cite: 40]
            other.block = nullptr; //[cite: 40]
        }

        // 新增：支持 MasterPtr<Derived> 到 MasterPtr<Base> 的隐式移动转换[cite: 40]
        template <typename U>
            requires std::convertible_to<U *, T *>
        MasterPtr(MasterPtr<U> &&other) noexcept : ptr(other.ptr), block(other.block)
        {
            other.ptr = nullptr;
            other.block = nullptr;
        }

        MasterPtr &operator=(MasterPtr &&other) noexcept //[cite: 40]
        {
            if (this != &other)
            {
                Reset();               //[cite: 40]
                ptr = other.ptr;       //[cite: 40]
                block = other.block;   //[cite: 40]
                other.ptr = nullptr;   //[cite: 40]
                other.block = nullptr; //[cite: 40]
            }
            return *this;
        }

        MasterPtr &operator=(std::nullptr_t) noexcept
        {
            Reset();
            return *this;
        }

        void Swap(MasterPtr &other) noexcept
        {
            std::swap(ptr, other.ptr);
            std::swap(block, other.block);
        }

        void Reset() noexcept //[cite: 40]
        {
            if (block) //[cite: 40]
            {
                // 1. release 标记置空，阻止后续新的 ObserverPtr::Lock() 成功[cite: 40]
                block->target_ptr.store(nullptr, std::memory_order_release); //[cite: 40]

                // 2. 无锁等待：仅当恰好有其他线程在 Lock() 内的微秒级临界区时极短暂 Yield[cite: 40]
                while (block->active_readers.load(std::memory_order_acquire) > 0) //[cite: 40]
                {
                    std::this_thread::yield(); //[cite: 40]
                }

                // 3. 安全物理释放[cite: 40]
                delete ptr;    //[cite: 40]
                ptr = nullptr; //[cite: 40]

                // 4. 减去 Master 持有的计数[cite: 40]
                if (block->observer_count.fetch_sub(1, std::memory_order_acq_rel) == 1) //[cite: 40]
                {
                    delete block; //[cite: 40]
                }
                block = nullptr; //[cite: 40]
            }
            else if (ptr) //[cite: 40]
            {
                delete ptr;    //[cite: 40]
                ptr = nullptr; //[cite: 40]
            }
        }

        [[nodiscard]] T *Get() const noexcept { return ptr; }              //[cite: 40]
        [[nodiscard]] T *operator->() const noexcept { return ptr; }       //[cite: 40]
        [[nodiscard]] T &operator*() const noexcept { return *ptr; }       //[cite: 40]
        explicit operator bool() const noexcept { return ptr != nullptr; } //[cite: 40]

        // 比较运算符补充
        template <typename U>
        bool operator==(const MasterPtr<U> &other) const noexcept { return ptr == other.Get(); }
        bool operator==(std::nullptr_t) const noexcept { return ptr == nullptr; }
    };

    // 工厂函数：提供类似 std::make_unique 的安全构造[cite: 40]
    template <typename T, typename... Args>
    [[nodiscard]] MasterPtr<T> MakeMaster(Args &&...args)
    {
        return MasterPtr<T>(new T(std::forward<Args>(args)...));
    }
}

// 哈希支持
template <typename T>
struct std::hash<RandEngine::Core::Memory::MasterPtr<T>>
{
    size_t operator()(const RandEngine::Core::Memory::MasterPtr<T> &p) const noexcept
    {
        return std::hash<T *>{}(p.Get());
    }
};