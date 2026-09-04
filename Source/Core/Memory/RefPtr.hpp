#pragma once
#include "RefCounted.hpp"
#include <concepts>
#include <utility>
#include <functional>

namespace RandomEngine::Core::Memory
{
    template <typename T>
    struct RefPtr
    {
    private:
        T* m_ptr = nullptr;

        template <typename U>
        friend class RefPtr;

    public:
        using element_type = T;

        constexpr RefPtr() noexcept = default;
        constexpr RefPtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}

        // 默认将传入的裸指针引用计数 +1；若为已加计数的指针可设 add_ref = false
        explicit RefPtr(T* ptr, bool add_ref = true) noexcept : m_ptr(ptr)
        {
            if (m_ptr && add_ref)
            {
                m_ptr->AddRef();
            }
        }

        // 拷贝构造
        RefPtr(const RefPtr& other) noexcept : m_ptr(other.m_ptr)
        {
            if (m_ptr)
            {
                m_ptr->AddRef();
            }
        }

        // 派生类到基类的隐式拷贝转换 (Derived -> Base)
        template <typename U>
            requires std::convertible_to<U*, T*>
        RefPtr(const RefPtr<U>& other) noexcept : m_ptr(other.m_ptr)
        {
            if (m_ptr)
            {
                m_ptr->AddRef();
            }
        }

        // 移动构造：零原子计数开销
        RefPtr(RefPtr&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        // 派生类到基类的隐式移动转换
        template <typename U>
            requires std::convertible_to<U*, T*>
        RefPtr(RefPtr<U>&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        ~RefPtr()
        {
            Reset();
        }

        RefPtr& operator=(const RefPtr& other) noexcept
        {
            if (this != &other)
            {
                RefPtr(other).Swap(*this);
            }
            return *this;
        }

        RefPtr& operator=(RefPtr&& other) noexcept
        {
            if (this != &other)
            {
                RefPtr(std::move(other)).Swap(*this);
            }
            return *this;
        }

        RefPtr& operator=(std::nullptr_t) noexcept
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

        void Swap(RefPtr& other) noexcept
        {
            std::swap(m_ptr, other.m_ptr);
        }

        [[nodiscard]] T* Get() const noexcept { return m_ptr; }
        [[nodiscard]] T* operator->() const noexcept { return m_ptr; }
        [[nodiscard]] T& operator*() const noexcept { return *m_ptr; }
        explicit operator bool() const noexcept { return m_ptr != nullptr; }

        template <typename U>
        bool operator==(const RefPtr<U>& other) const noexcept { return m_ptr == other.Get(); }
        bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }

        // 转换操作符
        template <typename U>
        [[nodiscard]] RefPtr<U> StaticCast() const noexcept
        {
            return RefPtr<U>(static_cast<U*>(m_ptr));
        }

        template <typename U>
        [[nodiscard]] RefPtr<U> DynamicCast() const noexcept
        {
            return RefPtr<U>(dynamic_cast<U*>(m_ptr));
        }
    };

    // 工厂创建函数
    template <typename T, typename... Args>
    [[nodiscard]] RefPtr<T> MakeIntrusive(Args&&... args)
    {
        return RefPtr<T>(new T(std::forward<Args>(args)...));
    }
}

// std::hash 特化支持
template <typename T>
struct std::hash<RandomEngine::Core::Memory::RefPtr<T>>
{
    size_t operator()(const RandomEngine::Core::Memory::RefPtr<T>& p) const noexcept
    {
        return std::hash<T*>{}(p.Get());
    }
};