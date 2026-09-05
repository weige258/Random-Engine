#pragma once
#include "IdLock.hpp"

namespace RandomEngine::Core::Memory {

    // ★ 纯通用代理:只依赖 ObserverPtr<T> 和 IdLock,
    //   不知道任何具体对象体系(可复用于任意资源系统)
    template <typename T>
    class IdLockedPtr {
    private:
        Core::Memory::ObserverPtr<T> m_obs;
        typename Core::Memory::ObserverPtr<T>::ScopedRef m_scoped;
        IdLock *m_lock = nullptr;

    public:
        IdLockedPtr() noexcept = default;

        IdLockedPtr(Core::Memory::ObserverPtr<T> obs, IdLock &lock)
            : m_obs(std::move(obs)),
              m_scoped(m_obs.Lock()),
              m_lock(&lock)
        { if (m_scoped) lock.Lock(ThisThreadToken()); }

        IdLockedPtr(IdLockedPtr &&o) noexcept
            : m_obs(std::move(o.m_obs)),
              m_scoped(std::move(o.m_scoped)),
              m_lock(o.m_lock)
            { o.m_lock = nullptr; }
        IdLockedPtr(const IdLockedPtr &)            = delete;
        IdLockedPtr &operator=(const IdLockedPtr &) = delete;

        ~IdLockedPtr() { if (m_lock) m_lock->Unlock(ThisThreadToken()); }

        [[nodiscard]] T *operator->() const noexcept { return m_scoped.Get(); }
        [[nodiscard]] T &operator*()  const noexcept { return *m_scoped.Get(); }
        [[nodiscard]] T *Get()        const noexcept { return m_scoped.Get(); }
        explicit operator bool()      const noexcept { return m_scoped.operator bool(); }
    };
}
