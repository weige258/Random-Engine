#pragma once
#include <atomic>
#include <cstdint>

namespace RandEngine::Core::Memory
{
    class RefCounted
    {
    private:
        // 引用计数内嵌在对象中
        mutable std::atomic<uint32_t> m_ref_count{0};

    protected:
        virtual ~RefCounted() = default;

    public:
        RefCounted() = default;

        // 对象拷贝/移动时，禁止拷贝引用计数
        RefCounted(const RefCounted&) noexcept {}
        RefCounted& operator=(const RefCounted&) noexcept { return *this; }
        RefCounted(RefCounted&&) noexcept {}
        RefCounted& operator=(RefCounted&&) noexcept { return *this; }

        void AddRef() const noexcept
        {
            m_ref_count.fetch_add(1, std::memory_order_relaxed);
        }

        void Release() const noexcept
        {
            if (m_ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                delete this;
            }
        }

        [[nodiscard]] uint32_t GetRefCount() const noexcept
        {
            return m_ref_count.load(std::memory_order_relaxed);
        }
    };
}