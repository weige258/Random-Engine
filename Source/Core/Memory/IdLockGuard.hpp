#pragma once   

#include "IdLock.hpp"
#include <vector>
#include <span>
#include <algorithm>
#include <cstdint>

// 统一锁守卫
// - 存桶号（uint32_t），非 IdLock* → 无悬空、无额外 allocation
// - 不可移动不可拷贝 → 杜绝跨线程 transfer
// - 多 ID 按桶号排序去重 → 单次调用内零死锁
// - 嵌套两次 LockOf 仍是 ABBA —— 需要同时锁多个 id 请用 LockOf(a,b,c) 一次给全

namespace RandomEngine::Core::Memory
{

    class IdLockGuard
    {
        std::vector<uint32_t> m_buckets;
        std::vector<IdLock> *m_pool = nullptr;
        uint32_t m_token = 0;

    public:
        IdLockGuard(std::vector<IdLock> &pool, uint32_t bucket, uint32_t token)
            : m_pool(&pool), m_token(token)
        {
            m_buckets.push_back(bucket);
            (*m_pool)[bucket].Lock(m_token);
        }

        IdLockGuard(std::vector<IdLock> &pool, std::span<const uint32_t> buckets, uint32_t token)
            : m_pool(&pool), m_token(token), m_buckets(buckets.begin(), buckets.end()) // 拷进来
        {
            std::sort(m_buckets.begin(), m_buckets.end());
            auto u = std::unique(m_buckets.begin(), m_buckets.end());
            m_buckets.erase(u, m_buckets.end());

            for (uint32_t b : m_buckets)
                (*m_pool)[b].Lock(m_token);
        }

        ~IdLockGuard()
        {
            for (size_t i = m_buckets.size(); i-- > 0;)
                (*m_pool)[m_buckets[i]].Unlock(m_token);
        }

        IdLockGuard(const IdLockGuard &) = delete;
        IdLockGuard &operator=(const IdLockGuard &) = delete;
        IdLockGuard(IdLockGuard &&) = delete;
        IdLockGuard &operator=(IdLockGuard &&) = delete;
    };

}