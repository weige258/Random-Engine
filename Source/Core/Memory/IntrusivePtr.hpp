#pragma once
#include "RefCounted.hpp"
#include <concepts>
#include <utility>
#include <functional>

namespace RandEngine::Core::Memory
{
    template <typename T>
    struct IntrusivePtr
    {
    private:
        T* m_ptr = nullptr;

        template <typename U>
        friend class IntrusivePtr;

    public:
        using element_type = T;

        constexpr IntrusivePtr() noexcept = default;
        constexpr IntrusivePtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}

        // 默认将传入的裸指针引用计数 +1；若为已加计数的指针可设 add_ref = false
        explicit IntrusivePtr(T* ptr, bool add_ref = true) noexcept : m_ptr(ptr)
        {
            if (m_ptr && add_ref)
            {
                m_ptr->AddRef();
            }
        }

        // 拷贝构造
        IntrusivePtr(const IntrusivePtr& other) noexcept : m_ptr(other.m_ptr)
        {
            if (m_ptr)
            {
                m_ptr->AddRef();
            }
        }

        // 派生类到基类的隐式拷贝转换 (Derived -> Base)
        template <typename U>
            requires std::convertible_to<U*, T*>
        IntrusivePtr(const IntrusivePtr<U>& other) noexcept : m_ptr(other.m_ptr)
        {
            if (m_ptr)
            {
                m_ptr->AddRef();
            }
        }

        // 移动构造：零原子计数开销
        IntrusivePtr(IntrusivePtr&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        // 派生类到基类的隐式移动转换
        template <typename U>
            requires std::convertible_to<U*, T*>
        IntrusivePtr(IntrusivePtr<U>&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        ~IntrusivePtr()
        {
            Reset();
        }

        IntrusivePtr& operator=(const IntrusivePtr& other) noexcept
        {
            if (this != &other)
            {
                IntrusivePtr(other).Swap(*this);
            }
            return *this;
        }

        IntrusivePtr& operator=(IntrusivePtr&& other) noexcept
        {
            if (this != &other)
            {
                IntrusivePtr(std::move(other)).Swap(*this);
            }
            return *this;
        }

        IntrusivePtr& operator=(std::nullptr_t) noexcept
        {
            Reset();
            return *this;
        }

        void Reset() noexcept
        {
            if (m_ptr)
            {
                T* temp = m_ptr;
                m_ptr = nullptr;
                temp->Release();
            }
        }

        void Swap(IntrusivePtr& other) noexcept
        {
            std::swap(m_ptr, other.m_ptr);
        }

        [[nodiscard]] T* Get() const noexcept { return m_ptr; }
        [[nodiscard]] T* operator->() const noexcept { return m_ptr; }
        [[nodiscard]] T& operator*() const noexcept { return *m_ptr; }
        explicit operator bool() const noexcept { return m_ptr != nullptr; }

        template <typename U>
        bool operator==(const IntrusivePtr<U>& other) const noexcept { return m_ptr == other.Get(); }
        bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }

        // 转换操作符
        template <typename U>
        [[nodiscard]] IntrusivePtr<U> StaticCast() const noexcept
        {
            return IntrusivePtr<U>(static_cast<U*>(m_ptr));
        }

        template <typename U>
        [[nodiscard]] IntrusivePtr<U> DynamicCast() const noexcept
        {
            return IntrusivePtr<U>(dynamic_cast<U*>(m_ptr));
        }
    };

    // 工厂创建函数
    template <typename T, typename... Args>
    [[nodiscard]] IntrusivePtr<T> MakeIntrusive(Args&&... args)
    {
        return IntrusivePtr<T>(new T(std::forward<Args>(args)...));
    }
}

// std::hash 特化支持
template <typename T>
struct std::hash<RandEngine::Core::Memory::IntrusivePtr<T>>
{
    size_t operator()(const RandEngine::Core::Memory::IntrusivePtr<T>& p) const noexcept
    {
        return std::hash<T*>{}(p.Get());
    }
};