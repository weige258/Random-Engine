#pragma once

#include "PtrControlBlock.hpp"
#include "ObserverPtr.hpp"
#include <utility>
#include <exception>
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

        explicit MasterPtr(T *resource = nullptr)
            : ptr(resource)
        {
            if (!ptr)
                return;

            try
            {
                block = new PtrControlBlock();
                block->state.store(
                    PtrControlBlock::OwnerUnit,
                    std::memory_order_relaxed);
                block->target_ptr.store(
                    static_cast<void *>(ptr),
                    std::memory_order_relaxed);

                block->owned_ptr = ptr;

                block->destroy_owned = [](void *p) noexcept
                {
                    delete static_cast<T *>(p);
                };
            }
            catch (...)
            {
                delete ptr;
                ptr = nullptr;
                throw;
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
            T *old_ptr = std::exchange(ptr, nullptr);

            PtrControlBlock *old_block =
                std::exchange(block, nullptr);

            if (!old_block)
            {
                delete old_ptr;
                return;
            }

            // O(1) 立即返回，不等任何人。
            // 销毁责任自动落到"最后一个放手的人"：
            //   · 此刻无读者 → Close() 内部直接销毁
            //   · 此刻有读者 → 最后一个 ScopedRef::Leave() 销毁
            old_block->Close();

            old_block->ReleaseOwnerRef();   // 释放 Master 那份引用；之后绝不再访问 old_block
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