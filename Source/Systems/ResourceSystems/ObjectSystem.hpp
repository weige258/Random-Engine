#pragma once
#include "Containers/SparseSet.hpp"
#include "Objects/BaseObject/BaseObject.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include "Behaviors/BaseBehavior/ISystemUpdateBehavior.hpp"
#include "Core/Memory/IdLock.hpp"
#include "Core/Cast/Cast.hpp"
#include "Systems/ISystem.hpp"
#include "Config.hpp"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <ranges>
#include <utility>
#include <vector>
#include "Core/Memory/IdLockGuard.hpp"

namespace RandomEngine::Systems::ResourceSystems
{

    class ObjectSystem : Systems::ISystem
    {
    private:
        // 对象容器
        RandomEngine::Core::Containers::SparseSet<Core::Memory::MasterPtr<Core::Objects::BaseObject>, RandomEngine::Core::Config::ObjectIDType> objects;

        // 对象删增锁
        mutable std::shared_mutex m_structure_mutex;
        // id锁
        std::vector<Core::Memory::IdLock> m_locks;

        [[nodiscard]] Core::Memory::IdLock &GetIdLock(const Core::Config::ObjectIDType &id) noexcept
        {
            return m_locks[static_cast<size_t>(id) % m_locks.size()];
        }

    public:
        // 获取对象

        Core::Objects::BaseObject &GetRef(const Core::Config::ObjectIDType &id)
        {
            return *objects.Get(id);
        }

        template <typename U>
        U &GetRef(const Core::Config::ObjectIDType &id)
        {
            return Core::Cast::ReCast<U &>(GetRef(id));
        }

        // 回调形态：进回调前加锁、出回调后解锁，锁的作用域严格等于回调体。
        // 因此本线程在回调体内最多持有一把 IdLock，「持有一把、再去等另一把」
        // 这个 ABBA 的必要前提写不出来。多对象交互按作用域串行调用多次即可；
        // 若必须同一时刻看到两者状态，用 LockOf(span) 一次给全。
        // 返回 false 表示对象不存在或类型不符，回调不会执行。
        template <typename Fn>
        bool GetLocked(Core::Config::ObjectIDType id, Fn &&fn)
        {
            Core::Memory::IdLock &lock = GetIdLock(id);
            const uint64_t token = Core::Memory::ThisThreadToken();
            lock.Lock(token);

            auto *mp = objects.GetPtr(id);
            if (!mp)
            {
                lock.Unlock(token);
                return false;
            }

            try
            {
                std::forward<Fn>(fn)(*mp->Get());
            }
            catch (...)
            {
                lock.Unlock(token);
                throw;
            }

            lock.Unlock(token);
            return true;
        }

        template <typename U, typename Fn>
        bool GetLocked(Core::Config::ObjectIDType id, Fn &&fn)
        {
            Core::Memory::IdLock &lock = GetIdLock(id);
            const uint64_t token = Core::Memory::ThisThreadToken();
            lock.Lock(token);

            auto *mp = objects.GetPtr(id);
            U *raw = mp ? Core::Cast::ReCast<U *>(mp->Get()) : nullptr;
            if (!raw)
            {
                lock.Unlock(token);
                return false;
            }

            try
            {
                std::forward<Fn>(fn)(*raw);
            }
            catch (...)
            {
                lock.Unlock(token);
                throw;
            }

            lock.Unlock(token);
            return true;
        }

        Core::Memory::ObserverPtr<Core::Objects::BaseObject> GetObserver(const Core::Config::ObjectIDType &id)
        {
            if (auto *master_ptr = objects.GetPtr(id))
            {
                return Core::Memory::ObserverPtr<Core::Objects::BaseObject>(*master_ptr);
            }

            return {};
        }

        template <typename U>
        Core::Memory::ObserverPtr<U> GetObserver(const Core::Config::ObjectIDType &id)
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            if (auto *master_ptr = objects.GetPtr(id))
            {
                if (Core::Cast::ReCast<U *>(master_ptr->Get()) != nullptr)
                {
                    return Core::Memory::ObserverPtr<U>(*master_ptr);
                }
            }
            return {};
        }

        auto GetAllRef() noexcept
        {
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            return std::views::iota(size_t(0), objects.Size()) | std::views::transform([ids, datas](size_t i) -> std::pair<Core::Config::ObjectIDType, Core::Objects::BaseObject &>
                                                                                       { return {ids[i], *datas[i]}; });
        }

        template <typename U>
        [[nodiscard]] auto GetAllRef() noexcept
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            return std::views::iota(size_t(0), objects.Size()) | std::views::filter([datas](size_t i)
                                                                                    { return datas[i] && Core::Cast::ReCast<U *>(datas[i].Get()) != nullptr; }) |
                   std::views::transform([ids, datas](size_t i) -> std::pair<Core::Config::ObjectIDType, U &>
                                         { return {ids[i], *Core::Cast::ReCast<U *>(datas[i].Get())}; });
        }

        [[nodiscard]] std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<Core::Objects::BaseObject>>> GetAllObserver() const
        {
            std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<Core::Objects::BaseObject>>> result;
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            result.reserve(datas.size());
            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (datas[i])
                {
                    result.emplace_back(ids[i], Core::Memory::ObserverPtr<Core::Objects::BaseObject>(datas[i]));
                }
            }
            return result;
        }

        template <typename U>
        std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<U>>> GetAllObserver() const
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<U>>> result;
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            result.reserve(datas.size());
            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (datas[i] && Core::Cast::ReCast<U *>(datas[i].Get()) != nullptr)
                {
                    result.emplace_back(ids[i], Core::Memory::ObserverPtr<U>(datas[i]));
                }
            }
            return result;
        }

        // 批量回调：一次快照 + 一次按桶升序的批量加锁，锁的作用域严格等于整轮回调。
        // 与 GetAllObserver 相比：无逐对象 ObserverPtr 的原子引用计数，也无逐个 GetLocked 的往返。
        // 回调体内不得再取任何锁，否则会重新引入「持有一把、再去等另一把」。
        template <typename Fn>
        void GetAllLocked(Fn &&fn)
        {
            const auto ids = objects.GetAllIDs();
            const auto datas = objects.GetAll();

            // 先按桶号升序去重占住全部桶，杜绝与其它线程 ABBA 死锁
            auto bucket_guard = LockOf(ids);

            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (!datas[i])
                    continue;

                std::forward<Fn>(fn)(ids[i], *datas[i].Get());
            }
        }

        template <typename U, typename Fn>
        void GetAllLocked(Fn &&fn)
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            const auto ids = objects.GetAllIDs();
            const auto datas = objects.GetAll();

            // 只收集匹配的对象，避免去锁与本次调用无关的桶
            std::vector<std::pair<Core::Config::ObjectIDType, U *>> matched;
            std::vector<Core::Config::ObjectIDType> matched_ids;
            matched.reserve(datas.size());
            matched_ids.reserve(datas.size());

            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (!datas[i])
                    continue;

                if (U *raw = Core::Cast::ReCast<U *>(datas[i].Get()))
                {
                    matched.emplace_back(ids[i], raw);
                    matched_ids.push_back(ids[i]);
                }
            }

            auto bucket_guard = LockOf(matched_ids);

            for (const auto &[id, raw] : matched)
            {
                std::forward<Fn>(fn)(id, *raw);
            }
        }

        // 添加对象
        template <typename U>
        Core::Config::ObjectIDType Add(U &&object)
        {
            std::unique_lock<std::shared_mutex> struct_lock(m_structure_mutex);
            using RawType = std::decay_t<U>;
            static_assert(std::is_base_of_v<Core::Objects::BaseObject, RawType>,
                          "U must derive from BaseObject!");

            Core::Memory::MasterPtr<Core::Objects::BaseObject> ptr(new RawType(std::move(object)));

            Core::Config::ObjectIDType id = objects.AllocateID();
            ptr->id = id;
            auto guard = LockOf(id);
            objects.Insert(id, std::move(ptr));
            return id;
        }

        template <typename... Args>
            requires(sizeof...(Args) > 1)
        std::vector<Core::Config::ObjectIDType> Add(Args &&...args)
        {
            std::vector<Core::Config::ObjectIDType> ids;
            ids.reserve(sizeof...(args));

            (ids.push_back(Add(std::forward<Args>(args))), ...);

            return ids;
        }

        std::vector<Core::Config::ObjectIDType> Add(std::vector<Core::Memory::MasterPtr<Core::Objects::BaseObject>> &&container)
        {
            std::unique_lock<std::shared_mutex> struct_lock(m_structure_mutex);
            std::vector<Core::Config::ObjectIDType> ids;
            ids.reserve(container.size());
 
            for (auto &ptr : container)
            {
                if (!ptr)
                    continue;

                Core::Config::ObjectIDType id = objects.AllocateID();
                ptr->id = id;
                auto guard = LockOf(id);
                objects.Insert(id, std::move(ptr));
                ids.push_back(id);
            }
            
            return ids;
        }

        // 删除对象
        bool Delete(const Core::Config::ObjectIDType id)
        {
            Core::Memory::MasterPtr<Core::Objects::BaseObject> retired;

            {
                std::unique_lock<std::shared_mutex> struct_lock(m_structure_mutex);
                auto guard = LockOf(id);

                auto *stored = objects.GetPtr(id);
                if (!stored)
                    return false;

                // 先移出所有权，再删槽位。
                retired = std::move(*stored);

                objects.Delete(id);
            }

            // 结构锁和 IdLock 均已释放，等待 Reader 退出后析构。
            retired.Reset();

            return true;
        }

        // 加锁

        [[nodiscard]] Core::Memory::IdLockGuard LockOf(Core::Config::ObjectIDType id)
        {
            uint32_t bucket = static_cast<uint32_t>(static_cast<size_t>(id) % m_locks.size());
            return Core::Memory::IdLockGuard(m_locks, bucket, Core::Memory::ThisThreadToken());
        }

        [[nodiscard]] Core::Memory::IdLockGuard LockOf(std::span<const Core::Config::ObjectIDType> ids)
        {
            std::vector<uint32_t> buckets;
            buckets.reserve(ids.size());
            for (auto id : ids)
                buckets.push_back(static_cast<uint32_t>(static_cast<size_t>(id) % m_locks.size()));

            return Core::Memory::IdLockGuard(m_locks, std::move(buckets), Core::Memory::ThisThreadToken());
        }

    public:
        void InitLockSystem(uint32_t cpu_l3_cache_kb);

        // 系统执行
        void Init(System &system);

        void Run(System &system);

        void Destroy();
    };
};