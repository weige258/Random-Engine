#pragma once

#include "PtrControlBlock.hpp"
#include "ObserverPtr.hpp"
#include <thread>
#include <utility>
#include <concepts>
#include <functional>

namespace RandomEngine::Core::Memory
{
    template <typename T>
    struct MasterPtr
    {
    private:
        T *ptr = nullptr;                 
        PtrControlBlock *block = nullptr; // 配合类型擦除后的控制块

        template <typename U>
        friend struct MasterPtr;

        template <typename U>
        friend class ObserverPtr;

    public:
        MasterPtr(std::nullptr_t) noexcept : ptr(nullptr), block(nullptr) {}

        explicit MasterPtr(T *resource = nullptr) : ptr(resource) 
        {
            if (ptr)
            {
                block = new PtrControlBlock();                                                
                block->target_ptr.store(static_cast<void *>(ptr), std::memory_order_relaxed); 
                block->observer_count.store(1, std::memory_order_relaxed);                    // Master 算 1 个生命周期屏障
            }
        }

        ~MasterPtr()
        {
            Reset(); 
        }

        MasterPtr(const MasterPtr &) = delete;            
        MasterPtr &operator=(const MasterPtr &) = delete; 

        MasterPtr(MasterPtr &&other) noexcept : ptr(other.ptr), block(other.block) 
        {
            other.ptr = nullptr;   
            other.block = nullptr; 
        }

        // 新增：支持 MasterPtr<Derived> 到 MasterPtr<Base> 的隐式移动转换
        template <typename U>
            requires std::convertible_to<U *, T *>
        MasterPtr(MasterPtr<U> &&other) noexcept : ptr(other.ptr), block(other.block)
        {
            other.ptr = nullptr;
            other.block = nullptr;
        }

        MasterPtr &operator=(MasterPtr &&other) noexcept 
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

        [[nodiscard]] T *Get() const noexcept { return ptr; }              
        [[nodiscard]] T *operator->() const noexcept { return ptr; }       
        [[nodiscard]] T &operator*() const noexcept { return *ptr; }       
        explicit operator bool() const noexcept { return ptr != nullptr; } 

        // 比较运算符补充
        template <typename U>
        bool operator==(const MasterPtr<U> &other) const noexcept { return ptr == other.Get(); }
        bool operator==(std::nullptr_t) const noexcept { return ptr == nullptr; }
    };

    // 工厂函数：提供类似 std::make_unique 的安全构造
    template <typename T, typename... Args>
    [[nodiscard]] MasterPtr<T> MakeMaster(Args &&...args)
    {
        return MasterPtr<T>(new T(std::forward<Args>(args)...));
    }
}

// 哈希支持
template <typename T>
struct std::hash<RandomEngine::Core::Memory::MasterPtr<T>>
{
    size_t operator()(const RandomEngine::Core::Memory::MasterPtr<T> &p) const noexcept
    {
        return std::hash<T *>{}(p.Get());
    }
};