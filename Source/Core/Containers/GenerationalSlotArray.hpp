#pragma once
#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace RandomEngine::Core::Containers
{
    // 句柄独立成命名空间级模板，不做成容器的嵌套类型：
    // 否则 PortId / Edge 想存句柄就得写成 GenerationalSlotArray<T,I,G>::Id，
    // 边表会被迫依赖具体的容器实例类型，基础结构就被污染了。
    template <typename IndexType = std::uint64_t, typename GenerationType = std::uint64_t>
    struct SlotId
    {
        static_assert(std::is_integral_v<IndexType> && std::is_unsigned_v<IndexType>,
                      "IndexType must be an unsigned integer");
        static_assert(std::is_integral_v<GenerationType> && std::is_unsigned_v<GenerationType>,
                      "GenerationType must be an unsigned integer");

        static constexpr IndexType kInvalidIndex = std::numeric_limits<IndexType>::max();
        static constexpr GenerationType kInvalidGeneration = GenerationType{0};

        IndexType index = kInvalidIndex;
        GenerationType generation = kInvalidGeneration;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return index != kInvalidIndex && generation != kInvalidGeneration;
        }
        [[nodiscard]] friend constexpr bool operator==(SlotId, SlotId) noexcept = default;
    };

    // 基础容器：只负责「句柄 -> 活对象」的解析与槽位复用判废。
    // 关系（边）、图、Schema、锁都不在这里；那些属于上层 BaseGraph。
    // 约定：DataType 析构不得抛异常（Remove / Clear 标了 noexcept）。
    //       DataType 需可移动构造，或先 Reserve 避免 vector 扩容搬移。
    // 线程：无内部同步，构图按单线程事务；多线程请各持一份实例或外层加锁。
    template <typename DataType,
              typename IndexType = std::uint64_t,
              typename GenerationType = std::uint32_t>
    struct GenerationalSlotArray
    {
    public:
        using Id = SlotId<IndexType, GenerationType>;
        using value_type = DataType;

    private:
        struct Slot
        {
            std::optional<DataType> value;
            GenerationType generation = static_cast<GenerationType>(1);
            bool retired = false;
        };

    private:
        std::vector<Slot> slots;
        std::vector<IndexType> free_indices;
        std::size_t size = 0;

    public:
        GenerationalSlotArray() = default;
        GenerationalSlotArray(const GenerationalSlotArray &) = delete;
        GenerationalSlotArray &operator=(const GenerationalSlotArray &) = delete;
        GenerationalSlotArray(GenerationalSlotArray &&) noexcept = default;
        GenerationalSlotArray &operator=(GenerationalSlotArray &&) noexcept = default;

        // 需要稳定地址时先 Reserve，或让 DataType 本身是 unique_ptr / MasterPtr。
        // std::vector 扩容会搬移 DataType，Resolve() 返回的指针只保证到下一次结构变更有效。
        void Reserve(std::size_t n)
        {
            slots.reserve(n);
            free_indices.reserve(n);
        }

        // ── ① 自动发号：把现成对象放进去，id 由容器给 ──────────────────
        // 优先复用 free_indices 里的空槽，没有才新增槽位。
        // 返回本次的 Id。原先的自动发号入口已删除，这就是它的替代。
        [[nodiscard]] Id Insert(DataType &&v)
        {
            return AutoInsert(std::move(v));
        }

        // 顺带补左值版本，否则 DataType obj; t.Insert(obj); 直接编不过。
        // 只要右值版本的话，删掉这一个即可。
        // 约束：DataType 必须是可拷贝构造的。move-only 类型（MasterPtr / unique_ptr）
        // 不参与这个重载，误传左值时会直接报「无匹配函数」，而不是沉进 optional 的深模板错误。
        [[nodiscard]] Id Insert(const DataType &v)
            requires std::copy_constructible<DataType>
        {
            return AutoInsert(v);
        }

        // ── ② 指定 id 强制落槽：从文件加载蓝图 / shader 图 ──────────────
        // 文件是权威。Insert(3,T) / Insert(32,T) / Insert(6,T) 无条件落到该槽，
        // 不校验代次、不需要快照、不需要先调任何东西；越界自动扩展。
        // 至少带一个 data 参数，避免单参数 Insert(3) 和上面的 Insert(T) 抢重载。
        template <typename... Args>
        [[nodiscard]] Id Insert(IndexType index, Args &&...args)
        {
            static_assert(sizeof...(Args) >= 1,
                          "Insert(index, ...) needs a value; use Insert(value) for auto-id");
            return ForceInsert(index, std::nullopt, std::forward<Args>(args)...);
        }

        // 若文件里连完整句柄一起 dump 了：无条件采用文件里的 generation。
        template <typename... Args>
        [[nodiscard]] Id Insert(Id id, Args &&...args)
        {
            static_assert(sizeof...(Args) >= 1,
                          "Insert(Id, ...) needs a value; use Insert(value) for auto-id");
            if (!id.IsValid())
                return Id{};
            return ForceInsert(id.index, id.generation, std::forward<Args>(args)...);
        }

        // 边表 / 文件层只有 index 时的解析入口（不做代次判废）。
        [[nodiscard]] DataType *ResolveByIndex(IndexType index) noexcept
        {
            return const_cast<DataType *>(
                static_cast<const GenerationalSlotArray *>(this)->ResolveByIndex(index));
        }
        [[nodiscard]] const DataType *ResolveByIndex(IndexType index) const noexcept
        {
            if (index == Id::kInvalidIndex ||
                static_cast<std::size_t>(index) >= slots.size())
                return nullptr;
            const Slot &s = slots[static_cast<std::size_t>(index)];
            return (s.value && !s.retired) ? &*s.value : nullptr;
        }
        [[nodiscard]] bool ContainsIndex(IndexType index) const noexcept
        {
            return ResolveByIndex(index) != nullptr;
        }

        bool Remove(Id id) noexcept
        {
            Slot *s = FindMutable(id);
            if (!s)
                return false;
            s->value.reset();

            // 删除即退役这一代：槽位可以复用，但旧句柄永远对不上。
            const GenerationType next = static_cast<GenerationType>(s->generation + GenerationType{1});
            if (next == Id::kInvalidGeneration) // 回绕：该槽位永久退役，不再复用
            {
                s->generation = Id::kInvalidGeneration;
                s->retired = true;
            }
            else
            {
                s->generation = next;
                free_indices.push_back(id.index);
            }
            --size;
            return true;
        }

        [[nodiscard]] DataType *Resolve(Id id) noexcept
        {
            return const_cast<DataType *>(
                static_cast<const GenerationalSlotArray *>(this)->Resolve(id));
        }

        [[nodiscard]] const DataType *Resolve(Id id) const noexcept
        {
            const Slot *s = Find(id);
            return s ? &*s->value : nullptr;
        }

        [[nodiscard]] bool Contains(Id id) const noexcept { return Resolve(id) != nullptr; }
        [[nodiscard]] std::size_t Size() const noexcept { return size; }
        [[nodiscard]] bool Empty() const noexcept { return size == 0; }
        [[nodiscard]] std::size_t SlotCount() const noexcept { return slots.size(); }

        // 清空但保留代次：旧句柄不会误命中之后新建的同 index 对象。
        void Clear() noexcept
        {
            free_indices.clear();
            for (std::size_t i = 0; i < slots.size(); ++i)
            {
                Slot &s = slots[i];
                if (s.retired)
                {
                    s.value.reset();
                    continue;
                }

                if (s.value)
                {
                    s.value.reset();
                    const GenerationType next =
                        static_cast<GenerationType>(s.generation + GenerationType{1});
                    if (next == Id::kInvalidGeneration)
                    {
                        s.generation = Id::kInvalidGeneration;
                        s.retired = true;
                        continue;
                    }
                    s.generation = next;
                }
                free_indices.push_back(static_cast<IndexType>(i));
            }
            size = 0;
        }

        // fn(Id, DataType&) / fn(Id, const DataType&)
        template <typename F>
        void ForEach(F &&fn)
        {
            for (std::size_t i = 0; i < slots.size(); ++i)
            {
                Slot &s = slots[i];
                if (s.value)
                    fn(Id{static_cast<IndexType>(i), s.generation}, *s.value);
            }
        }
        template <typename F>
        void ForEach(F &&fn) const
        {
            for (std::size_t i = 0; i < slots.size(); ++i)
            {
                const Slot &s = slots[i];
                if (s.value)
                    fn(Id{static_cast<IndexType>(i), s.generation}, *s.value);
            }
        }

    private:
        // Insert(T) 的实现：自动挑槽 + 原位构造。构造抛异常时容器零改动。
        template <typename U>
        [[nodiscard]] Id AutoInsert(U &&v)
        {
            IndexType index;
            if (!free_indices.empty())
            {
                index = free_indices.back(); // 先不 pop：构造抛异常时槽位不丢
                Slot &s = slots[static_cast<std::size_t>(index)];
                assert(!s.value && !s.retired && "free slot must be empty");
                s.value.emplace(std::forward<U>(v));
                free_indices.pop_back();
            }
            else
            {
                assert(slots.size() < static_cast<std::size_t>(Id::kInvalidIndex));
                index = static_cast<IndexType>(slots.size());
                // 保证 free_indices 容量 >= slots.size()：此后 Remove / Clear 的
                // push_back 永不重分配，它们标 noexcept 才成立。
                free_indices.reserve(slots.size() + 1);
                slots.emplace_back(); // generation 默认 1
                try
                {
                    slots.back().value.emplace(std::forward<U>(v));
                }
                catch (...)
                {
                    slots.pop_back(); // 回滚，容器状态完全不变
                    throw;
                }
            }
            ++size;
            return Id{index, slots[static_cast<std::size_t>(index)].generation};
        }

        // 两个指定 id 的 Insert 重载共用。file_gen 有值 = 无条件采用文件代次；
        // 无值 = 沿用槽当前代次（新补的槽为 1）。
        template <typename... Args>
        [[nodiscard]] Id ForceInsert(IndexType index,
                                     std::optional<GenerationType> file_gen,
                                     Args &&...args)
        {
            static_assert(std::constructible_from<DataType, Args &&...>,
                          "DataType must be constructible from the given args; "
                          "move-only types such as MasterPtr must be passed as rvalues");
            if (index == Id::kInvalidIndex)
                return Id{};
            if (file_gen && *file_gen == Id::kInvalidGeneration)
                return Id{};                       // gen=0 是无效保留值

            // 先构造：抛异常时容器一个字段都没动
            std::optional<DataType> tmp;
            tmp.emplace(std::forward<Args>(args)...);

            const std::size_t u = static_cast<std::size_t>(index);
            if (u >= slots.size())
            {
                const std::size_t old = slots.size();
                slots.resize(u + 1);               // 新槽：gen=1、空、未退役
                free_indices.reserve(slots.size());
                for (std::size_t i = old; i < slots.size(); ++i)
                    free_indices.push_back(static_cast<IndexType>(i));
            }

            Slot &s = slots[u];
            if (s.retired)
                return Id{};                       // 回绕槽永不复活

            if (file_gen)
                s.generation = *file_gen;          // ← 强制：不校验当前值

            const bool was_empty = !s.value;
            if (!was_empty)
                s.value.reset();                   // 强制替换

            const auto it = std::find(free_indices.begin(), free_indices.end(), index);
            if (it != free_indices.end())
                free_indices.erase(it);

            s.value.emplace(std::move(*tmp));      // 只要求 DataType 可移动构造
            if (was_empty)
                ++size;
            return Id{index, s.generation};
        }

        [[nodiscard]] const Slot *Find(Id id) const noexcept
        {
            if (!id.IsValid())
                return nullptr;
            // 用 IndexType 侧比较，避免 64 位 index 在 32 位 size_t 上被截断成小值误命中
            if (id.index >= static_cast<IndexType>(slots.size()))
                return nullptr;
            const Slot &s = slots[static_cast<std::size_t>(id.index)];
            if (!s.value || s.retired || s.generation != id.generation)
                return nullptr;
            return &s;
        }
        [[nodiscard]] Slot *FindMutable(Id id) noexcept
        {
            return const_cast<Slot *>(
                static_cast<const GenerationalSlotArray *>(this)->Find(id));
        }
    };
}