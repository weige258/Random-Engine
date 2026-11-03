#pragma once   

#include "IdLock.hpp"

#include <vector>
#include <span>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

// 统一锁守卫
// - 存桶号（uint32_t），非 IdLock* → 无悬空、无额外 allocation
// - 不可移动不可拷贝 → 杜绝跨线程 transfer
// - 多 ID 按桶号排序去重 → 单次调用内零死锁
// - 嵌套两次 LockOf 仍是 ABBA —— 需要同时锁多个 id 请用 LockOf(a,b,c) 一次给全

namespace RandomEngine::Core::Memory
{

    class IdLockGuard
    {
    private:
        std::vector<IdLock> *m_pool = nullptr;

        // 多桶模式使用。
        std::vector<uint32_t> m_buckets;

        // 单桶模式不需要 vector 分配。
        uint32_t m_single_bucket = 0;

        uint64_t m_token = 0;

        size_t m_acquired = 0;

        bool m_single = false;

        // 直接持有单把 IdLock 时使用，仍属于同一个守卫。
        IdLock *m_direct_lock = nullptr;

        void Release() noexcept
        {
            if (m_direct_lock)
            {
                m_direct_lock->Unlock(m_token);
                m_direct_lock = nullptr;
                return;
            }

            if (!m_pool)
                return;

            if (m_single)
            {
                m_pool->at(m_single_bucket).Unlock(m_token);
            }
            else
            {
                while (m_acquired > 0)
                {
                    --m_acquired;
                    (*m_pool)[m_buckets[m_acquired]].Unlock(m_token);
                }
            }

            m_pool = nullptr;
            m_acquired = 0;
        }

        void AcquireSorted()
        {
            std::sort(m_buckets.begin(), m_buckets.end());

            m_buckets.erase(
                std::unique(m_buckets.begin(), m_buckets.end()),
                m_buckets.end());

            // 获取前先验证全部桶号，避免中途发现非法索引。
            for (uint32_t bucket : m_buckets)
            {
                if (bucket >= m_pool->size())
                    throw std::out_of_range("IdLockGuard: bucket out of range");
            }

            for (uint32_t bucket : m_buckets)
            {
                (*m_pool)[bucket].Lock(m_token);
                ++m_acquired;
            }
        }

    public:
        // 单桶：无 vector 分配。
        IdLockGuard(
            std::vector<IdLock> &pool,
            uint32_t bucket,
            uint64_t token)
            : m_pool(&pool),
              m_single_bucket(bucket),
              m_token(token),
              m_single(true)
        {
            if (token == 0 || bucket >= pool.size())
                throw std::invalid_argument("IdLockGuard: invalid bucket/token");

            pool[bucket].Lock(token);
            m_acquired = 1;
        }

        // 直接锁一把 IdLock。仍然是同一个 IdLockGuard 类型。
        explicit IdLockGuard(IdLock &lock, uint64_t token)
            : m_token(token),
              m_direct_lock(&lock)
        {
            if (token == 0)
                throw std::invalid_argument("IdLockGuard: token cannot be zero");

            lock.Lock(token);
        }

        // 多桶：复制后排序、去重。
        IdLockGuard(
            std::vector<IdLock> &pool,
            std::span<const uint32_t> buckets,
            uint64_t token)
            : m_pool(&pool),
              m_buckets(buckets.begin(), buckets.end()),
              m_token(token)
        {
            if (token == 0)
                throw std::invalid_argument("IdLockGuard: token cannot be zero");

            AcquireSorted();
        }

        // 多桶：直接接收已经构建好的 vector，避免再次复制。
        IdLockGuard(
            std::vector<IdLock> &pool,
            std::vector<uint32_t> &&buckets,
            uint64_t token)
            : m_pool(&pool),
              m_buckets(std::move(buckets)),
              m_token(token)
        {
            if (token == 0)
                throw std::invalid_argument("IdLockGuard: token cannot be zero");

            AcquireSorted();
        }

        ~IdLockGuard() noexcept
        {
            Release();
        }

        IdLockGuard(const IdLockGuard &) = delete;
        IdLockGuard &operator=(const IdLockGuard &) = delete;

        // Guard 不允许移动到其他线程。
        IdLockGuard(IdLockGuard &&) = delete;
        IdLockGuard &operator=(IdLockGuard &&) = delete;
    };

}