#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace RandomEngine::Core::Containers
{
    constexpr size_t NOT_FOUND = size_t(-1);

    template <typename U>
    struct OptVal
    {
        using type = U;
    }; // 剥 optional 壳
    template <typename U>
    struct OptVal<std::optional<U>>
    {
        using type = U;
    };
    template <typename U>
    using ValT = typename OptVal<U>::type;

    template <typename U>
    struct IsOpt : std::false_type
    {
    };
    template <typename U>
    struct IsOpt<std::optional<U>> : std::true_type
    {
    };

    template <typename T>
    struct IsIdx : std::bool_constant<std::is_copy_constructible_v<ValT<T>>>
    {
    };

    template <typename T>
    constexpr size_t ColIndexOfImpl() { return NOT_FOUND; }

    template <typename T, typename H, typename... R>
    constexpr size_t ColIndexOfImpl()
    {
        if constexpr (std::is_same_v<ValT<T>, ValT<H>>)
            return 0;
        else
        {
            constexpr size_t r = ColIndexOfImpl<T, R...>();
            return r == NOT_FOUND ? NOT_FOUND : 1 + r;
        }
    }

    template <typename... TS>
    struct StableRowTable
    {
    private:
        static constexpr size_t N = sizeof...(TS);
        static constexpr uint32_t npos = 0xFFFFFFFF;
        static constexpr size_t CHUNK = 4096;

    public:
        template <typename T>
        static constexpr size_t ColIndexOf() { return ColIndexOfImpl<T, TS...>(); }

        struct Row
        {
            std::tuple<TS...> v;
            uint32_t no = npos; // 自身行号
            uint32_t next_free = npos;
            uint8_t alive = 0;                   // ★无 handle:验活 = 读此位
            std::array<uint32_t, N> back{};      // 值块内下标(O(1) 删除用)
            std::array<uint32_t, N> back_null{}; // null 链内下标

            template <typename T>
            auto &Get()
            {
                static_assert(ColIndexOf<T>() != NOT_FOUND, "get<T>: T 不是本表列类型");
                return std::get<ColIndexOf<T>()>(v);
            }

            template <size_t I>
            auto &At() { return std::get<I>(v); }

            const std::tuple<TS...> &Data() const { return v; } // ★tuple 直取
        };

    private:
        std::vector<std::unique_ptr<Row[]>> pool_;
        uint32_t count_ = 0, free_head_ = npos;

        template <typename U>
        using ColMap = std::unordered_map<ValT<U>, std::vector<Row *>>;
        std::tuple<ColMap<TS>...> idx_; // 值 → 行指针桶(重复值=多元素)

        template <typename>
        using RowVec = std::vector<Row *>; // ★新增:让 TS 能进模式
        std::tuple<RowVec<TS>...> nulls_;  // ★第 92 行改为这个

        Row &R(uint32_t n) { return pool_[n / CHUNK][n % CHUNK]; }
        template <size_t I>
        using Col = std::tuple_element_t<I, std::tuple<TS...>>;

        template <size_t I>
        void AttachOne(Row *r)
        {
            if constexpr (!IsIdx<Col<I>>::value)
            { // 所有权列:不进任何索引
                r->back[I] = npos;
                r->back_null[I] = npos;
            }
            else
            {
                auto &raw = std::get<I>(r->v);
                if constexpr (IsOpt<Col<I>>::value)
                {
                    if (raw)
                    {
                        auto &vec = std::get<I>(idx_)[*raw];
                        r->back[I] = (uint32_t)vec.size();
                        r->back_null[I] = npos;
                        vec.push_back(r);
                    }
                    else
                    {
                        auto &nv = std::get<I>(nulls_);
                        r->back[I] = npos;
                        r->back_null[I] = (uint32_t)nv.size();
                        nv.push_back(r);
                    }
                }
                else
                {
                    auto &vec = std::get<I>(idx_)[raw];
                    r->back[I] = (uint32_t)vec.size();
                    vec.push_back(r);
                }
            }
        }

        template <size_t... Is>
        void RollbackAttach(Row *r, std::index_sequence<Is...>)
        {
            ((r->back[Is] != npos || r->back_null[Is] != npos
                  ? DetachOne<Is>(r)
                  : void(0)),
             ...);
        }

        template <size_t I>
        void DetachOne(Row *r)
        {
            if constexpr (!IsIdx<Col<I>>::value)
            {
                // 所有权列:无索引可摘
            }
            else
            {
                auto &raw = std::get<I>(r->v);
                if constexpr (IsOpt<Col<I>>::value)
                {
                    if (!raw)
                    {
                        auto &nv = std::get<I>(nulls_);
                        uint32_t p = r->back_null[I];
                        if (p != nv.size() - 1)
                        {
                            nv[p] = nv.back();
                            nv[p]->back_null[I] = p;
                        }
                        nv.pop_back();
                        return;
                    }
                }
                auto key = [&]() -> ValT<Col<I>> &
                {
                    if constexpr (IsOpt<Col<I>>::value)
                        return *raw;
                    else
                        return raw;
                }();
                auto &m = std::get<I>(idx_);
                auto it = m.find(key);
                auto &vec = it->second;
                uint32_t p = r->back[I];
                if (p != vec.size() - 1)
                {
                    vec[p] = vec.back();
                    vec[p]->back[I] = p;
                }
                vec.pop_back();
                if (vec.empty())
                    m.erase(it);
            }
        }

        template <size_t... Is>
        void AttachAll(Row *r, std::index_sequence<Is...>) { (AttachOne<Is>(r), ...); }

        template <size_t... Is>
        void DetachAll(Row *r, std::index_sequence<Is...>) { (DetachOne<Is>(r), ...); }

        template <size_t I, size_t... Is> // Delete<U> 专用:列 I 已整桶移除
        void DetachExcept(Row *r, std::index_sequence<Is...>)
        {
            ((Is == I ? void(0) : DetachOne<Is>(r)), ...);
        }

        template <size_t I>
        void KillRow(Row *r)
        { // 行死于"列 I 整桶删除"路径
            DetachExcept<I>(r, std::make_index_sequence<N>{});
            r->alive = 0;
            r->next_free = free_head_;
            free_head_ = r->no;
        }

        template <size_t... Is>
        void ClearImpl(std::index_sequence<Is...>)
        {
            (std::get<Is>(idx_).clear(), ...);
            (std::get<Is>(nulls_).clear(), ...);
            for (auto &p : pool_)
                for (size_t i = 0; i < CHUNK; ++i)
                    p[i].alive = 0;
            free_head_ = npos;
            count_ = 0;
        }

    public:
        // 插入
        bool Insert(TS... v)
        {
            if (count_ == npos)
                return false;

            uint32_t n;
            bool reused = (free_head_ != npos);
            if (reused)
            {
                n = free_head_;
                free_head_ = R(n).next_free;
            }
            else
            {
                if (count_ % CHUNK == 0)
                {
                    try
                    {
                        pool_.push_back(std::make_unique<Row[]>(CHUNK));
                    }
                    catch (...)
                    {
                        return false;
                    }
                }
                n = count_++;
            }

            Row &r = R(n);

            std::tuple<TS...> data;
            try
            {
                data = std::tuple<TS...>(std::move(v)...);
            }
            catch (...)
            {
                if (reused)
                {
                    R(n).next_free = free_head_;
                    free_head_ = n;
                }
                else
                    --count_;
                return false;
            }

            r.v = std::move(data);
            r.no = n;
            r.alive = 1;
            r.back.fill(npos);
            r.back_null.fill(npos);

            try
            {
                AttachAll(&r, std::make_index_sequence<N>{});
            }
            catch (...)
            {
                RollbackAttach(&r, std::make_index_sequence<N>{});
                r.alive = 0;
                r.next_free = free_head_;
                free_head_ = n;
                return false;
            }
            return true;
        }

        // 删除
        bool Delete(Row *r)
        {
            if (!r || !r->alive) // 两个真实失败点:空指针 / 已死行
                return false;
            DetachAll(r, std::make_index_sequence<N>{});
            r->alive = 0;
            r->next_free = free_head_;
            free_head_ = r->no;
            return true;
        }

        template <typename U>
        size_t Delete(U u)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "Delete<U>: U 必须是本表列类型");
            auto &m = std::get<I>(idx_);
            auto it = m.find(u);
            if (it == m.end())
                return 0;
            std::vector<Row *> victims = std::move(it->second); // 整桶 move 防迭代失效
            m.erase(it);
            for (Row *r : victims)
                KillRow<I>(r);
            return victims.size();
        }

        template <typename U>
        size_t Delete(std::nullopt_t)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "Delete<U>: U 必须是本表列类型");
            static_assert(IsOpt<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "该列非 optional,不存在 null");
            auto &nv = std::get<I>(nulls_);
            if (nv.empty())
                return 0;
            std::vector<Row *> victims = std::move(nv);
            nv.clear();
            for (Row *r : victims)
                KillRow<I>(r);
            return victims.size();
        }

        template <typename U>
        bool DeleteOne(const U &u)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "DeleteOne<U>: U 必须是本表列类型");
            auto &m = std::get<I>(idx_);
            auto it = m.find(u);
            if (it == m.end() || it->second.empty())
                return false;
            return Delete(it->second.front());
        }

        // 查询
        template <typename U>
        std::vector<std::tuple<TS...>> Find(const U &u)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "FindTuples<U>: U 必须是本表列类型");
            std::vector<std::tuple<TS...>> out;
            auto it = std::get<I>(idx_).find(u);
            if (it != std::get<I>(idx_).end())
                for (Row *r : it->second)
                    out.push_back(r->v);
            return out;
        }

        template <typename U>
        std::vector<std::tuple<TS...>> Find(std::nullopt_t)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "FindTuples<U>: U 必须是本表列类型");
            static_assert(IsOpt<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "该列非 optional,不存在 null");
            std::vector<std::tuple<TS...>> out;
            for (Row *r : std::get<I>(nulls_))
                out.push_back(r->v);
            return out;
        }

        template <typename U>
        std::vector<Row *> FindRows(const U &u)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "FindRows<U>: U 必须是本表列类型");
            std::vector<Row *> out;
            auto it = std::get<I>(idx_).find(u);
            if (it != std::get<I>(idx_).end())
                out = it->second; // 整桶拷贝指针(8B×k),与桶解耦
            return out;
        }

        template <typename U>
        std::vector<Row *> FindRows(std::nullopt_t)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "FindRows<U>: U 必须是本表列类型");
            static_assert(IsOpt<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "该列非 optional,不存在 null");
            return std::get<I>(nulls_); // 拷贝 null 链
        }

        // 遍历
        template <typename U, typename F>
        void ForEach(const U &u, F &&f)
        {
            constexpr size_t I = ColIndexOf<U>();
            static_assert(IsIdx<std::tuple_element_t<I, std::tuple<TS...>>>::value,
                          "所有权列(不可拷贝)不能作为查询/删除的键");
            static_assert(I != NOT_FOUND, "ForEach<U>: U 必须是本表列类型");
            auto it = std::get<I>(idx_).find(u);
            if (it == std::get<I>(idx_).end())
                return;
            auto vec = it->second; // 拷贝防遍历中删
            for (Row *r : vec)
                if (r->alive)
                    f(*r);
        }

        template <typename F>
        void ForEachAll(F &&f)
        {
            for (auto &p : pool_)
                for (size_t i = 0; i < CHUNK; ++i)
                    if (p[i].alive)
                        f(p[i]);
        }

        void Clear() { ClearImpl(std::make_index_sequence<N>{}); }
        uint32_t Size() const { return count_; }
    };
}