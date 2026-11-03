#pragma once

#include "IdLock.hpp"

#include <utility>
#include <exception>

namespace RandomEngine::Core::Memory
{
    template <typename T>
    class IdLockedPtr
    {
    private:
        T *m_ptr = nullptr;
        IdLock *m_lock = nullptr;

        // 获取锁时的原始 Token。
        uint64_t m_token = 0;

        void CheckThread() const noexcept
        {
#ifndef NDEBUG
            if (m_lock && ThisThreadToken() != m_token)
                std::terminate();
#endif
        }

        void Release() noexcept
        {
            if (m_lock)
            {
                CheckThread();

                m_lock->Unlock(m_token);

                m_lock = nullptr;
                m_ptr = nullptr;
                m_token = 0;
            }
        }

    public:
        struct adopt_lock_t {};

        IdLockedPtr() noexcept = default;

        IdLockedPtr(
            T *ptr,
            IdLock &lock,
            uint64_t token,
            adopt_lock_t) noexcept
            : m_ptr(ptr),
              m_lock(&lock),
              m_token(token)
        {
            if (token == 0)
                std::terminate();
#ifndef NDEBUG
            if (token != ThisThreadToken())
                std::terminate();
#endif
        }

        // 兼容原来 {ptr, lock, adopt_lock_t{}} 的调用方式。
        IdLockedPtr(
            T *ptr,
            IdLock &lock,
            adopt_lock_t) noexcept
            : IdLockedPtr(ptr, lock, ThisThreadToken(), adopt_lock_t{})
        {
        }

        IdLockedPtr(IdLockedPtr &&other) noexcept
        {
            // 禁止把正在持有的锁跨线程转移。
            other.CheckThread();

            m_ptr = std::exchange(other.m_ptr, nullptr);
            m_lock = std::exchange(other.m_lock, nullptr);
            m_token = std::exchange(other.m_token, 0);
        }

        IdLockedPtr &operator=(IdLockedPtr &&other) noexcept
        {
            if (this != &other)
            {
                Release();
                other.CheckThread();

                m_ptr = std::exchange(other.m_ptr, nullptr);
                m_lock = std::exchange(other.m_lock, nullptr);
                m_token = std::exchange(other.m_token, 0);
            }

            return *this;
        }

        IdLockedPtr(const IdLockedPtr &) = delete;
        IdLockedPtr &operator=(const IdLockedPtr &) = delete;

        ~IdLockedPtr() noexcept
        {
            Release();
        }

        [[nodiscard]] T *operator->() const noexcept
        {
            CheckThread();
            return m_ptr;
        }

        [[nodiscard]] T &operator*() const noexcept
        {
            CheckThread();
            return *m_ptr;
        }

        [[nodiscard]] T *Get() const noexcept
        {
            CheckThread();
            return m_ptr;
        }

        explicit operator bool() const noexcept
        {
            CheckThread();
            return m_ptr != nullptr;
        }
    };
}