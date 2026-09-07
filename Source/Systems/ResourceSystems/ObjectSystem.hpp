#pragma once
#include "Containers/SparseSet.hpp"
#include "Objects/BaseObject/BaseObject.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include "Behaviors/BaseBehavior/ISystemUpdateBehavior.hpp"
#include "Core/Memory/IdLock.hpp"
#include "Core/Memory/IdLockedPtr.hpp"
#include "Systems/ISystem.hpp"
#include "Config.hpp"
#include <memory>
#include <span>
#include <ranges>

namespace RandomEngine::Systems::ResourceSystems
{
    class ObjectSystem : Systems::ISystem
    {
    private:
        RandomEngine::Core::Containers::SparseSet<Core::Memory::MasterPtr<Core::Objects::BaseObject>, RandomEngine::Core::Config::ObjectIDType> objects;

        std::vector<Core::Memory::IdLock> m_locks;

        [[nodiscard]] Core::Memory::IdLock &LockOf(Core::Config::ObjectIDType id) noexcept
        {
            // ObjectIDType 若为非整型类,在此取其 index/generation 字段做映射
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
            return dynamic_cast<U &>(GetRef(id));
        }

        auto GetLocked(Core::Config::ObjectIDType id)
            -> Core::Memory::IdLockedPtr<Core::Objects::BaseObject>
        {
            Core::Memory::IdLock &lock = LockOf(id);
            lock.Lock(Core::Memory::ThisThreadToken());
            if (auto *mp = objects.GetPtr(id))
                return { mp->Get(), lock, typename Core::Memory::IdLockedPtr<Core::Objects::BaseObject>::adopt_lock_t{} };
            lock.Unlock(Core::Memory::ThisThreadToken());
            return {};
        }

        template <typename U>
        auto GetLocked(Core::Config::ObjectIDType id) -> Core::Memory::IdLockedPtr<U>
        {
            Core::Memory::IdLock &lock = LockOf(id);
            lock.Lock(Core::Memory::ThisThreadToken());
            if (auto *mp = objects.GetPtr(id))
            {
                if (U *raw = dynamic_cast<U *>(mp->Get()))
                    return { raw, lock, typename Core::Memory::IdLockedPtr<U>::adopt_lock_t{} };
                lock.Unlock(Core::Memory::ThisThreadToken());
                return {};
            }
            lock.Unlock(Core::Memory::ThisThreadToken());
            return {};
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
                if (dynamic_cast<U *>(master_ptr->Get()) != nullptr)
                {
                    return Core::Memory::ObserverPtr<U>(*master_ptr);
                }
            }
            return {};
        }

        [[nodiscard]] auto GetAll() noexcept
        {
            return objects.GetAll() | std::views::transform([](auto &master_ptr) -> Core::Objects::BaseObject &
                                                            { return *master_ptr; });
        }

        std::vector<Core::Memory::ObserverPtr<Core::Objects::BaseObject>> GetAllObserver() const
        {
            std::vector<Core::Memory::ObserverPtr<Core::Objects::BaseObject>> result;
            auto raw_span = objects.GetAll();
            result.reserve(raw_span.size());
            for (const auto &ptr : raw_span)
            {
                if (ptr)
                    result.emplace_back(ptr);
            }
            return result;
        }

        template <typename U>
        auto GetAll()
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            return objects.GetAll() | std::views::filter([](const auto &ptr)
                                                         { return ptr && dynamic_cast<U *>(ptr.Get()) != nullptr; }) |
                   std::views::transform([](const auto &ptr) -> U &
                                         { return *dynamic_cast<U *>(ptr.Get()); });
        }

        template <typename U>
        std::vector<Core::Memory::ObserverPtr<U>> GetAllObserver() const
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            std::vector<Core::Memory::ObserverPtr<U>> result;
            auto raw_span = objects.GetAll();
            result.reserve(raw_span.size());

            for (const auto &ptr : raw_span)
            {
                if (ptr && dynamic_cast<U *>(ptr.Get()) != nullptr)
                {
                    result.emplace_back(ptr);
                }
            }
            return result;
        }

        auto GetAllWithID() noexcept
        {
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            return std::views::iota(size_t(0), objects.Size()) | std::views::transform([ids, datas](size_t i) -> std::pair<Core::Config::ObjectIDType, Core::Objects::BaseObject &>
                                                                                       { return {ids[i], *datas[i]}; });
        }

        template <typename U>
        [[nodiscard]] auto GetAllWithID()
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            return std::views::iota(size_t(0), objects.Size()) | std::views::filter([datas](size_t i)
                                                                                    { return datas[i] && dynamic_cast<U *>(datas[i].Get()) != nullptr; }) |
                   std::views::transform([ids, datas](size_t i) -> std::pair<Core::Config::ObjectIDType, U &>
                                         { return {ids[i], *dynamic_cast<U *>(datas[i].Get())}; });
        }

        [[nodiscard]] std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<Core::Objects::BaseObject>>> GetAllObserverWithID() const
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
        std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<U>>> GetAllObserverWithID() const
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<U>>> result;
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            result.reserve(datas.size());
            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (datas[i] && dynamic_cast<U *>(datas[i].Get()) != nullptr)
                {
                    result.emplace_back(ids[i], Core::Memory::ObserverPtr<U>(datas[i]));
                }
            }
            return result;
        }

        // 添加对象
        template <typename U>
        Core::Config::ObjectIDType Add(U &&object)
        {
            using RawType = std::decay_t<U>;
            static_assert(std::is_base_of_v<Core::Objects::BaseObject, RawType>,
                          "U must derive from BaseObject!");

            Core::Memory::MasterPtr<Core::Objects::BaseObject> ptr(new RawType(std::move(object)));

            Core::Config::ObjectIDType id = objects.AllocateID();
            ptr->id = id;
            Core::Memory::IdLock &lock = LockOf(id);
            lock.Lock(Core::Memory::ThisThreadToken());
            objects.Insert(id, std::move(ptr));
            lock.Unlock(Core::Memory::ThisThreadToken());
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
            std::vector<Core::Config::ObjectIDType> ids;
            ids.reserve(container.size());

            for (auto &ptr : container)
            {
                if (!ptr)
                    continue;

                Core::Config::ObjectIDType id = objects.AllocateID();
                ptr->id = id;
                Core::Memory::IdLock &lock = LockOf(id);
                lock.Lock(Core::Memory::ThisThreadToken());
                objects.Insert(id, std::move(ptr));
                lock.Unlock(Core::Memory::ThisThreadToken());
                ids.push_back(id);
            }

            return ids;
        }

        // 删除对象
        bool Delete(const Core::Config::ObjectIDType id)
        {
            Core::Memory::IdLock &lock = LockOf(id);
            lock.Lock(Core::Memory::ThisThreadToken());
            bool ok = objects.Delete(id);
            lock.Unlock(Core::Memory::ThisThreadToken());
            return ok;
        }

    public:
        // 系统执行
        void Init(System &system)
        {
            m_locks = std::vector<Core::Memory::IdLock>(65536);
        }

        void Run(System &system)
        {
        }

        void Destroy()
        {
        }
    };
};