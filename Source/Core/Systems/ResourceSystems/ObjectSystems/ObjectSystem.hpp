#pragma once
#include "Containers/SparseSet.hpp"
#include "Objects/BaseObject/BaseObject.hpp"
#include "Config.hpp"
#include <memory>
#include <span>
#include <ranges>

namespace RandEngine::Core::Systems::ResourceSystems::ObjectSystems
{
    class ObjectSystem
    {
    private:
        RandEngine::Core::Containers::SparseSet<std::shared_ptr<Objects::BaseObject>, RandEngine::Core::Config::ObjectIDType> objects;

    public:
        // 获取对象
        Objects::BaseObject &Get(const Config::ObjectIDType &id)
        {
            return *objects.Get(id);
        }

        template <typename U>
        U &Get(const Config::ObjectIDType &id)
        {
            return *std::static_pointer_cast<U>(objects.Get(id));
        }

        Objects::BaseObject *GetPtr(const Config::ObjectIDType &id)
        {
            auto *ptr = objects.GetPtr(id);
            if (!ptr)
                return nullptr;

            return ptr->get();
        }

        template <typename U>
        U *GetPtr(const Config::ObjectIDType &id)
        {
            auto *ptr = objects.GetPtr(id);
            if (!ptr)
                return nullptr;

            return std::static_pointer_cast<U>(*ptr).get();
        }

        auto GetAll() noexcept
        {
            return objects.GetAll() | std::views::transform([](const std::shared_ptr<Objects::BaseObject> &ptr) -> Objects::BaseObject &
                                                            { return *ptr; });
        }

        std::vector<Objects::BaseObject *> GetAll() const
        {
            std::vector<Objects::BaseObject *> result;
            auto raw_span = objects.GetAll();
            result.reserve(raw_span.size());
            for (const auto &ptr : raw_span)
            {
                if (ptr)
                    result.push_back(ptr.get());
            }
            return result;
        }

        template <typename U>
        auto GetAll()
        {
            static_assert(std::is_base_of_v<Objects::BaseObject, U>, "U must derive from BaseObject!");

            return objects.GetAll() | std::views::filter([](const std::shared_ptr<Objects::BaseObject> &ptr)
                                                         { return ptr && dynamic_cast<U *>(ptr.get()) != nullptr; }) |
                   std::views::transform([](const std::shared_ptr<Objects::BaseObject> &ptr) -> U &
                                         { return *static_cast<U *>(ptr.get()); });
        }

        template <typename U>
        std::vector<U *> GetAll() const
        {
            static_assert(std::is_base_of_v<Objects::BaseObject, U>, "U must derive from Objects::BaseObject!");

            std::vector<U *> result;
            auto raw_span = objects.GetAll();

            result.reserve(raw_span.size());

            for (const auto &ptr : raw_span)
            {
                if (ptr)
                {
                    if (auto *derived = dynamic_cast<U *>(ptr.get()))
                    {
                        result.push_back(derived);
                    }
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
            static_assert(std::is_base_of_v<Objects::BaseObject, U>, "U must derive from BaseObject!");

            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            return std::views::iota(size_t(0), objects.Size()) | std::views::filter([datas](size_t i)
                                                                                    { return datas[i] && dynamic_cast<U *>(datas[i].get()) != nullptr; }) |
                   std::views::transform([ids, datas](size_t i) -> std::pair<Config::ObjectIDType, U &>
                                         { return {ids[i], *static_cast<U *>(datas[i].get())}; });
        }

        [[nodiscard]] std::vector<std::pair<Config::ObjectIDType, Objects::BaseObject *>> GetAllWithID() const
        {
            std::vector<std::pair<Config::ObjectIDType, Objects::BaseObject *>> result;
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            result.reserve(datas.size());
            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (datas[i])
                {
                    result.emplace_back(ids[i], datas[i].get());
                }
            }
            return result;
        }

        template <typename U>
        std::vector<std::pair<Config::ObjectIDType, U *>> GetAllWithID() const
        {
            static_assert(std::is_base_of_v<Objects::BaseObject, U>, "U must derive from Objects::BaseObject!");

            std::vector<std::pair<Config::ObjectIDType, U *>> result;
            auto ids = objects.GetAllIDs();
            auto datas = objects.GetAll();

            result.reserve(datas.size());
            for (size_t i = 0; i < datas.size(); ++i)
            {
                if (datas[i])
                {
                    if (auto *derived = dynamic_cast<U *>(datas[i].get()))
                    {
                        result.emplace_back(ids[i], derived);
                    }
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

            auto ptr = std::make_shared<RawType>(std::forward<U>(object));

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

        std::vector<Config::ObjectIDType> Add(const std::vector<std::shared_ptr<Objects::BaseObject>> &container)
        {
            std::vector<Config::ObjectIDType> ids;
            ids.reserve(container.size());

            for (const auto &ptr : container)
            {
                if (!ptr)
                    continue;

                Config::ObjectIDType id = objects.AllocateID();
                ptr->id = id;
                objects.Insert(id, ptr);
                ids.push_back(id);
            }

            return ids;
        }

        // 删除对象
        bool Delete(const Config::ObjectIDType id)
        {
            return objects.Delete(id);
        }
    };
};
