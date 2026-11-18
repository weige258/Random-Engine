#pragma once
#include <shared_mutex>

namespace RandomEngine::Core::Memory
{

    template <typename T>
    struct RWLock
    {
    private:
        T value;
        mutable std::shared_mutex mutex;

    public:
        RWLock() = default;

        template <typename... Args,
                  typename = std::enable_if_t<
                      !(sizeof...(Args) == 1 && std::is_same_v<std::decay_t<Args>..., RWLock>)>>
            >
            explicit RWLock(Args &&...args) : value(std::forward<Args>(args)...) {}

        RWLock(const RWLock &) = delete;

        RWLock &operator=(const RWLock &) = delete;

        RWLock(RWLock &&) = delete;

        RWLock &operator=(RWLock &&) = delete;

        ~RWLock() = default;

        RWLockReadRef<T> Read() const
        {
            return RWLockReadRef<T>(*this);
        }

        RWLockWriteRef<T> Write()
        {
            return RWLockWriteRef<T>(*this);
        }
    };

    template <typename T>
    struct RWLockReadRef
    {
    private:
        const RWLock<T> &owner;

    public:
        explicit RWLockReadRef(const RWLock<T> &lock) : owner(lock)
        {
            owner.mutex.lock_shared();
        }

        ~RWLockReadRef()
        {
            owner.mutex.unlock_shared();
        }

        const T *operator->() const
        {
            return &owner.value;
        }

        const T &operator*() const
        {
            return owner.value;
        }

        RWLockReadRef(const RWLockReadRef &) = delete;
        RWLockReadRef &operator=(const RWLockReadRef &) = delete;
        RWLockReadRef(RWLockReadRef &&) = delete;
        RWLockReadRef &operator=(RWLockReadRef &&) = delete;
    };

    template <typename T>
    struct RWLockWriteRef
    {
        explicit RWLockWriteRef(RWLock<T> &lock) : owner(lock)
        {
            owner.mutex.lock();
        }

        ~RWLockWriteRef()
        {
            owner.mutex.unlock();
        }

        T* operator->()
        {
            return &owner.value;
        }

        T& operator*()
        {
            return owner.value;
        }

        T* operator->() const
        {
            return &owner.value;
        }

        T& operator*() const
        {
            return owner.value;
        }

        RWLockWriteRef(const RWLockWriteRef &) = delete;
        RWLockWriteRef &operator=(const RWLockWriteRef &) = delete;
        RWLockWriteRef(RWLockWriteRef &&) = delete;
        RWLockWriteRef &operator=(RWLockWriteRef &&) = delete;
    };
}