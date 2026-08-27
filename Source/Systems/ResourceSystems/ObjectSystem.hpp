#pragma once
#include "Containers/SparseSet.hpp"
#include "Objects/BaseObject/BaseObject.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include "Behaviors/BaseBehavior/ISystemUpdateBehavior.hpp"
#include "Systems/ISystem.hpp"
#include "Config.hpp"
#include <memory>
#include <span>
#include <ranges>

namespace RandEngine::Systems::ResourceSystems
{
    class ObjectSystem:Systems::ISystem
    {
    private:
        RandEngine::Core::Containers::SparseSet<Core::Memory::MasterPtr<Core::Objects::BaseObject>, RandEngine::Core::Config::ObjectIDType> objects;

    public:
        // 获取对象
        Core::Objects::BaseObject &Get(const Core::Config::ObjectIDType &id)
        {
            return *objects.Get(id);
        }

        template <typename U>
        U &Get(const Core::Config::ObjectIDType &id)
        {
            return dynamic_cast<U &>(Get(id));
        }

        Core::Memory::ObserverPtr<Core::Objects::BaseObject> GetPtr(const Core::Config::ObjectIDType &id)
        {
            if (auto *master_ptr = objects.GetPtr(id))
            {
                return Core::Memory::ObserverPtr<Core::Objects::BaseObject>(*master_ptr);
            }

            return {};
        }

        template <typename U>
        Core::Memory::ObserverPtr<U> GetPtr(const Core::Config::ObjectIDType &id)
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

        std::vector<Core::Memory::ObserverPtr<Core::Objects::BaseObject>> GetAllPtr() const
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
        std::vector<Core::Memory::ObserverPtr<U>> GetAllPtr() const
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

        [[nodiscard]] std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<Core::Objects::BaseObject>>> GetAllPtrWithID() const
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
        std::vector<std::pair<Core::Config::ObjectIDType, Core::Memory::ObserverPtr<U>>> GetAllPtrWithID() const
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
            std::vector<Core::Config::ObjectIDType> ids;
            ids.reserve(container.size());

            for (auto &ptr : container)
            {
                if (!ptr)
                    continue;

                Core::Config::ObjectIDType id = objects.AllocateID();
                ptr->id = id;
                objects.Insert(id, std::move(ptr)); // 完美匹配 SparseSet::Insert(id, DataType&&)
                ids.push_back(id);
            }

            return ids;
        }

        // 删除对象
        bool Delete(const Core::Config::ObjectIDType id)
        {
            return objects.Delete(id);
        }

    public:
        // 系统执行
        void Init(System& system)
        {
        }

        void Run(System& system)
        {

        }

        void Destroy()
        {
        }
    };
};