#pragma once
#include "IdLock.hpp"

namespace RandomEngine::Core::Memory {

    // ★ 终局版: 裸 T* + IdLock, 不再依赖 ObserverPtr/ScopedRef/block 协议
    //   锁由 Get 持有, 构造时 adopt_lock; 析构时 Unlock
    template <typename T>
    class IdLockedPtr {
    private:
        T *m_ptr = nullptr;
        IdLock *m_lock = nullptr;

    public:
        struct adopt_lock_t {};

        IdLockedPtr() noexcept = default;

        // ★ 锁已由调用方持有, 仅接管裸指针
        IdLockedPtr(T *ptr, IdLock &lock, adopt_lock_t) noexcept
            : m_ptr(ptr), m_lock(&lock) {}

        IdLockedPtr(IdLockedPtr &&o) noexcept
            : m_ptr(o.m_ptr), m_lock(o.m_lock)
        { o.m_ptr = nullptr; o.m_lock = nullptr; }

        IdLockedPtr(const IdLockedPtr &)            = delete;
        IdLockedPtr &operator=(const IdLockedPtr &) = delete;

        ~IdLockedPtr() { if (m_lock) m_lock->Unlock(ThisThreadToken()); }

        [[nodiscard]] T *operator->() const noexcept { return m_ptr; }
        [[nodiscard]] T &operator*()  const noexcept { return *m_ptr; }
        [[nodiscard]] T *Get()        const noexcept { return m_ptr; }
        explicit operator bool()      const noexcept { return m_ptr != nullptr; }
    };
}