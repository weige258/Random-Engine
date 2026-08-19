#pragma once
#include "Containers/SparseSet.hpp"
#include "Objects/BaseObject/BaseObject.hpp"
#include "Memory/MasterPtr.hpp"
#include "Memory/ObserverPtr.hpp"
#include "Behaviors/BaseBehavior/ISystemUpdateBehavior.hpp"
#include "Config.hpp"
#include <memory>
#include <span>
#include <ranges>

namespace RandEngine::Core::Systems::ResourceSystems::ObjectSystems
{
    class ObjectSystem
    {
    private:
        RandEngine::Core::Containers::SparseSet<Memory::MasterPtr<Objects::BaseObject>, RandEngine::Core::Config::ObjectIDType> objects;

    public:
        // 获取对象
        Objects::BaseObject &Get(const Config::ObjectIDType &id)
        {
            return *objects.Get(id);
        }

        template <typename U>
        U &Get(const Config::ObjectIDType &id)
        {
            return dynamic_cast<U &>(Get(id));
        }

        Memory::ObserverPtr<Objects::BaseObject> GetPtr(const Config::ObjectIDType &id)
        {
            if (auto *master_ptr = objects.GetPtr(id))
            {
                return Memory::ObserverPtr<Objects::BaseObject>(*master_ptr);
            }

            return {};
        }

        template <typename U>
        Memory::ObserverPtr<U> GetPtr(const Config::ObjectIDType &id)
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");
            if (auto *master_ptr = objects.GetPtr(id))
            {
                if (dynamic_cast<U *>(master_ptr->Get()) != nullptr)
                {
                    return Memory::ObserverPtr<U>(*master_ptr);
                }
            }
            return {};
        }

        [[nodiscard]] auto GetAll() noexcept
        {
            return objects.GetAll() | std::views::transform([](auto &master_ptr) -> Objects::BaseObject &
                                                            { return *master_ptr; });
        }

        std::vector<Memory::ObserverPtr<Objects::BaseObject>> GetAllPtr() const
        {
            std::vector<Memory::ObserverPtr<Objects::BaseObject>> result;
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
        std::vector<Memory::ObserverPtr<U>> GetAllPtr() const
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            std::vector<Memory::ObserverPtr<U>> result;
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

            return std::views::iota(size_t(0), objects.Size()) | std::views::transform([ids, datas](size_t i) -> std::pair<Config::ObjectIDType, Objects::BaseObject &>
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
                   std::views::transform([ids, datas](size_t i) -> std::pair<Config::ObjectIDType, U &>
                                         { return {ids[i], *dynamic_cast<U *>(datas[i].Get())}; });
        }

        [[nodiscard]] std::vector<std::pair<Config::ObjectIDType, Memory::ObserverPtr<Objects::BaseObject>>> GetAllPtrWithID() const
        {
            std::vector<std::pair<Config::ObjectIDType, Memory::ObserverPtr<Objects::BaseObject>>> result;
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            result.reserve(datas.size());
            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (datas[i])
                {
                    result.emplace_back(ids[i], Memory::ObserverPtr<Objects::BaseObject>(datas[i]));
                }
            }
            return result;
        }

        template <typename U>
        std::vector<std::pair<Config::ObjectIDType, Memory::ObserverPtr<U>>> GetAllPtrWithID() const
        {
            static_assert(std::is_polymorphic_v<U>, "U must be a polymorphic type!");

            std::vector<std::pair<Config::ObjectIDType, Memory::ObserverPtr<U>>> result;
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            result.reserve(datas.size());
            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (datas[i] && dynamic_cast<U *>(datas[i].Get()) != nullptr)
                {
                    result.emplace_back(ids[i], Memory::ObserverPtr<U>(datas[i]));
                }
            }
            return result;
        }

        // 添加对象
        template <typename U>
        Config::ObjectIDType Add(U &&object)
        {
            using RawType = std::decay_t<U>;
            static_assert(std::is_base_of_v<Objects::BaseObject, RawType>,
                          "U must derive from BaseObject!");

            Memory::MasterPtr<Objects::BaseObject> ptr(new RawType(std::forward<U>(object)));

            Config::ObjectIDType id = objects.AllocateID();
            ptr->id = id;
            objects.Insert(id, std::move(ptr));
            return id;
        }

        template <typename... Args>
            requires(sizeof...(Args) > 1)
        std::vector<Config::ObjectIDType> Add(Args &&...args)
        {
            std::vector<Config::ObjectIDType> ids;
            ids.reserve(sizeof...(args));

            (ids.push_back(Add(std::forward<Args>(args))), ...);

            return ids;
        }

        std::vector<Config::ObjectIDType> Add(std::vector<Memory::MasterPtr<Objects::BaseObject>> &&container)
        {
            std::vector<Config::ObjectIDType> ids;
            ids.reserve(container.size());

            for (auto &ptr : container)
            {
                if (!ptr)
                    continue;

                Config::ObjectIDType id = objects.AllocateID();
                ptr->id = id;
                objects.Insert(id, std::move(ptr)); // 完美匹配 SparseSet::Insert(id, DataType&&)
                ids.push_back(id);
            }

            return ids;
        }

        // 删除对象
        bool Delete(const Config::ObjectIDType id)
        {
            return objects.Delete(id);
        }

    public:
        // 系统执行
        void Init()
        {
        }

        void Run()
        {

        }

        void Destroy()
        {
        }
    };
};
