#include <cstdint>
#include <vector>
#include <cassert>
#include <stdexcept>
#include <span>
#include <type_traits>

namespace RandomEngine::Core::Containers
{

    template <typename DataType, typename IndexType = uint64_t>
    struct SparseSet
    {
    private:
        static_assert(std::is_integral_v<IndexType>, "IndexType must be an integral type!");

        static constexpr IndexType null_index = static_cast<IndexType>(-1);

        std::vector<IndexType> sparse;          // ID -> dense 下标 (空闲时为 null_index)
        std::vector<IndexType> dense_indices;   // dense 下标 -> ID (Swap-and-Pop 映射)
        std::vector<DataType> dense_data;       // 纯粹连续的 DataType 内存块
        std::vector<IndexType> free_ids;        // 稠密：连续存放空闲 ID (Swap-and-Pop)
        std::vector<IndexType> free_reverse;    // 稀疏：free_reverse[id] = id 在 free_ids 中的下标，null_index 表示不在空闲集

    public:
        SparseSet() = default;

        explicit SparseSet(IndexType init_size)
        {
            if constexpr (std::is_signed_v<IndexType>)
            {
                assert(init_size >= 0 && "Initial capacity cannot be negative!");
            }

            size_t cap = static_cast<size_t>(init_size);

            sparse.reserve(cap);
            dense_indices.reserve(cap);
            dense_data.reserve(cap);
            free_ids.reserve(cap);
            free_reverse.reserve(cap);
        }

        void Reserve(size_t n)
        {
            sparse.reserve(n);
            dense_indices.reserve(n);
            dense_data.reserve(n);
            free_ids.reserve(n);
            free_reverse.reserve(n);
        }

        IndexType AllocateID()
        {
            if (!free_ids.empty())
            {
                IndexType id = free_ids.back();
                free_reverse[static_cast<size_t>(id)] = null_index;
                free_ids.pop_back();
                sparse[static_cast<size_t>(id)] = null_index;
                return id;
            }
            assert(sparse.size() < static_cast<size_t>(null_index) && "Index overflow!");
            IndexType id = static_cast<IndexType>(sparse.size());
            sparse.push_back(null_index);
            free_reverse.push_back(null_index);
            return id;
        }

        [[nodiscard]] bool Contains(IndexType id) const noexcept
        {
            return Probe(sparse, dense_indices, dense_data, id) != nullptr;
        }

        [[nodiscard]] bool IsFree(IndexType id) const noexcept
        {
            if constexpr (std::is_signed_v<IndexType>)
            {
                if (id < 0) return false;
            }
            size_t u = static_cast<size_t>(id);
            return u < free_reverse.size() && free_reverse[u] != null_index;
        }

        void Insert(IndexType id, const DataType &data)
        {
            if (id == null_index) [[unlikely]]
                throw std::invalid_argument("Reserved null_index cannot be used as a valid ID!");

            if (auto *ptr = TryGetImpl(id))
            {
                *ptr = data;
                return;
            }

            size_t u_id = static_cast<size_t>(id);

            bool was_free = false;
            if (u_id >= sparse.size())
            {
                sparse.resize(u_id + 1, null_index);
                free_reverse.resize(u_id + 1, null_index);
            }
            else
            {
                was_free = IsFree(id);
            }

            dense_data.push_back(data);
            try
            {
                dense_indices.push_back(id);
            }
            catch (...)
            {
                dense_data.pop_back();
                throw;
            }
            if (was_free) RemoveFromFreeSet(id);
            sparse[u_id] = static_cast<IndexType>(dense_data.size() - 1);
        }

        void Insert(IndexType id, DataType &&data)
        {
            if (id == null_index) [[unlikely]]
                throw std::invalid_argument("Reserved null_index cannot be used as a valid ID!");

            if (auto *ptr = TryGetImpl(id))
            {
                *ptr = std::move(data);
                return;
            }

            size_t u_id = static_cast<size_t>(id);

            bool was_free = false;
            if (u_id >= sparse.size())
            {
                sparse.resize(u_id + 1, null_index);
                free_reverse.resize(u_id + 1, null_index);
            }
            else
            {
                was_free = IsFree(id);
            }

            dense_data.push_back(std::move(data));
            try
            {
                dense_indices.push_back(id);
            }
            catch (...)
            {
                dense_data.pop_back();
                throw;
            }
            if (was_free) RemoveFromFreeSet(id);
            sparse[u_id] = static_cast<IndexType>(dense_data.size() - 1);
        }

        IndexType Insert(const DataType &data)
        {
            IndexType id = AllocateID();

            try
            {
                dense_data.push_back(data);
            }
            catch (...)
            {
                free_ids.push_back(id);
                free_reverse[static_cast<size_t>(id)] = static_cast<IndexType>(free_ids.size() - 1);
                throw;
            }

            try
            {
                dense_indices.push_back(id);
            }
            catch (...)
            {
                dense_data.pop_back();
                free_ids.push_back(id);
                free_reverse[static_cast<size_t>(id)] = static_cast<IndexType>(free_ids.size() - 1);
                throw;
            }
            sparse[static_cast<size_t>(id)] = static_cast<IndexType>(dense_data.size() - 1);
            return id;
        }

        IndexType Insert(DataType &&data)
        {
            IndexType id = AllocateID();

            try
            {
                dense_data.push_back(std::move(data));
            }
            catch (...)
            {
                free_ids.push_back(id);
                free_reverse[static_cast<size_t>(id)] = static_cast<IndexType>(free_ids.size() - 1);
                throw;
            }

            try
            {
                dense_indices.push_back(id);
            }
            catch (...)
            {
                dense_data.pop_back();
                free_ids.push_back(id);
                free_reverse[static_cast<size_t>(id)] = static_cast<IndexType>(free_ids.size() - 1);
                throw;
            }
            sparse[static_cast<size_t>(id)] = static_cast<IndexType>(dense_data.size() - 1);
            return id;
        }

        DataType &Get(IndexType id)
        {
            if (auto *ptr = TryGetImpl(id))
                return *ptr;
            throw std::out_of_range("Invalid ID for Get operation!");
        }

        const DataType &Get(IndexType id) const
        {
            if (auto *ptr = TryGetImpl(id))
                return *ptr;
            throw std::out_of_range("Invalid ID for Get operation!");
        }

        DataType *GetPtr(IndexType id) noexcept
        {
            return TryGetImpl(id);
        }

        const DataType *GetPtr(IndexType id) const noexcept
        {
            return TryGetImpl(id);
        }

        std::vector<DataType> Get(const std::vector<IndexType> &ids) const
        {
            std::vector<DataType> result;
            result.reserve(ids.size());
            for (const auto &id : ids)
            {
                if (const auto *ptr = Probe(sparse, dense_indices, dense_data, id))
                    result.push_back(*ptr);
            }
            return result;
        }

        [[nodiscard]] std::vector<DataType *> GetPtrs(std::span<const IndexType> ids)
        {
            std::vector<DataType *> result;
            result.reserve(ids.size());
            for (auto id : ids)
                result.push_back(TryGetImpl(id));
            return result;
        }

        [[nodiscard]] std::vector<const DataType *> GetPtrs(std::span<const IndexType> ids) const
        {
            std::vector<const DataType *> result;
            result.reserve(ids.size());
            for (auto id : ids)
                result.push_back(TryGetImpl(id));
            return result;
        }

        IndexType GetIndex(const DataType *ptr) const noexcept
        {
            if (!ptr || dense_data.empty())
                return null_index;

            const DataType *start = dense_data.data();
            const DataType *end = start + dense_data.size();

            if (ptr >= start && ptr < end &&
                (reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(start)) % sizeof(DataType) == 0)
            {
                size_t dense_idx = static_cast<size_t>(ptr - start);
                return dense_indices[dense_idx];
            }

            return null_index;
        }

        IndexType GetIndex(const DataType &data) const noexcept
        {
            return GetIndex(&data);
        }

        std::vector<IndexType> GetIndices(const std::vector<DataType> &datas) const
        {
            std::vector<IndexType> result;
            result.reserve(datas.size());

            if (dense_data.empty())
            {
                result.resize(datas.size(), null_index);
                return result;
            }

            const DataType *start = dense_data.data();
            const DataType *end = start + dense_data.size();

            for (const auto &item : datas)
            {
                const DataType *ptr = &item;

                if (ptr >= start && ptr < end &&
                    (reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(start)) % sizeof(DataType) == 0)
                {
                    size_t dense_idx = static_cast<size_t>(ptr - start);
                    result.push_back(dense_indices[dense_idx]);
                }
                else
                {
                    IndexType found_id = null_index;

                    for (size_t i = 0; i < dense_data.size(); ++i)
                    {
                        if constexpr (requires(const DataType &a, const DataType &b) { { a == b } -> std::convertible_to<bool>; })
                        {
                            if (dense_data[i] == item)
                            {
                                found_id = dense_indices[i];
                                break;
                            }
                        }
                    }

                    result.push_back(found_id);
                }
            }

            return result;
        }

        std::vector<IndexType> GetIndices(const std::vector<const DataType *> &data_ptrs) const
        {
            std::vector<IndexType> result;
            result.reserve(data_ptrs.size());

            for (const auto *ptr : data_ptrs)
            {
                result.push_back(GetIndex(ptr));
            }

            return result;
        }

        std::vector<IndexType> GetIndices(std::span<const DataType *const> data_ptrs) const
        {
            std::vector<IndexType> result;
            result.reserve(data_ptrs.size());

            for (const auto *ptr : data_ptrs)
            {
                result.push_back(GetIndex(ptr));
            }

            return result;
        }

        std::vector<IndexType> GetIndices(std::span<const DataType> datas) const
        {
            std::vector<IndexType> result;
            result.reserve(datas.size());

            if (dense_data.empty())
            {
                result.resize(datas.size(), null_index);
                return result;
            }

            const DataType *start = dense_data.data();
            const DataType *end = start + dense_data.size();

            for (const auto &item : datas)
            {
                const DataType *ptr = &item;

                if (ptr >= start && ptr < end &&
                    (reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(start)) % sizeof(DataType) == 0)
                {
                    size_t dense_idx = static_cast<size_t>(ptr - start);
                    result.push_back(dense_indices[dense_idx]);
                }
                else
                {
                    IndexType found_id = null_index;
                    for (size_t i = 0; i < dense_data.size(); ++i)
                    {
                        if constexpr (requires(const DataType &a, const DataType &b) { { a == b } -> std::convertible_to<bool>; })
                        {
                            if (dense_data[i] == item)
                            {
                                found_id = dense_indices[i];
                                break;
                            }
                        }
                    }
                    result.push_back(found_id);
                }
            }
            return result;
        }

        std::vector<IndexType> GetIndices(std::span<DataType *const> data_ptrs) const
        {
            std::vector<IndexType> result;
            result.reserve(data_ptrs.size());
            for (const auto *ptr : data_ptrs)
            {
                result.push_back(GetIndex(ptr));
            }
            return result;
        }

        std::vector<IndexType> GetIndices(std::initializer_list<const DataType *> data_ptrs) const
        {
            return GetIndices(std::span<const DataType *const>(data_ptrs.begin(), data_ptrs.size()));
        }

        [[nodiscard]] std::span<DataType> GetAll() noexcept
        {
            return dense_data;
        }

        [[nodiscard]] std::span<const DataType> GetAll() const noexcept
        {
            return dense_data;
        }

        [[nodiscard]] std::span<const IndexType> GetAllIDs() const noexcept
        {
            return dense_indices;
        }

        template <typename Predicate>
        std::vector<IndexType> GetIndicesIf(Predicate &&pred) const
        {
            std::vector<IndexType> result;

            for (size_t i = 0; i < dense_data.size(); ++i)
            {
                if (pred(dense_data[i]))
                {
                    result.push_back(dense_indices[i]);
                }
            }

            return result;
        }

        bool Delete(IndexType id) noexcept(std::is_nothrow_move_assignable_v<DataType>)
        {
            if (!Contains(id))
                return false;

            size_t u_id = static_cast<size_t>(id);
            size_t remove_idx = static_cast<size_t>(sparse[u_id]);
            size_t last_idx = dense_data.size() - 1;

            if (remove_idx != last_idx)
            {
                dense_data[remove_idx] = std::move(dense_data.back());
                dense_indices[remove_idx] = dense_indices.back();
                sparse[static_cast<size_t>(dense_indices[remove_idx])] = static_cast<IndexType>(remove_idx);
            }

            dense_data.pop_back();
            dense_indices.pop_back();

            sparse[u_id] = null_index;
            free_reverse[u_id] = static_cast<IndexType>(free_ids.size());
            free_ids.push_back(id);

            return true;
        }

        size_t Delete(std::span<const IndexType> ids) noexcept(std::is_nothrow_move_assignable_v<DataType>)
        {
            size_t deleted_count = 0;
            for (IndexType id : ids)
            {
                if (Delete(id))
                {
                    ++deleted_count;
                }
            }
            return deleted_count;
        }

        size_t Delete(std::initializer_list<IndexType> ids) noexcept(std::is_nothrow_move_assignable_v<DataType>)
        {
            return Delete(std::span<const IndexType>(ids.begin(), ids.size()));
        }

        bool Delete(const DataType *ptr) noexcept(std::is_nothrow_move_assignable_v<DataType>)
        {
            if (!ptr || dense_data.empty())
                return false;

            const DataType *start = dense_data.data();
            const DataType *end = start + dense_data.size();

            if (ptr >= start && ptr < end)
            {
                uintptr_t offset = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(start);
                if (offset % sizeof(DataType) == 0)
                {
                    size_t dense_idx = static_cast<size_t>(ptr - start);
                    return Delete(dense_indices[dense_idx]);
                }
            }

            return false;
        }

        bool Delete(const DataType &object) noexcept(std::is_nothrow_move_assignable_v<DataType>)
        {
            return Delete(&object);
        }

        template <typename Predicate>
        size_t DeleteIf(Predicate &&pred)
        {
            size_t deleted_count = 0;
            size_t i = 0;
            while (i < dense_data.size())
            {
                if (pred(dense_data[i]))
                {
                    size_t remove_idx = i;
                    size_t last_idx = dense_data.size() - 1;
                    IndexType id = dense_indices[remove_idx];

                    if (remove_idx != last_idx)
                    {
                        dense_data[remove_idx] = std::move(dense_data.back());
                        dense_indices[remove_idx] = dense_indices.back();
                        sparse[static_cast<size_t>(dense_indices[remove_idx])] = static_cast<IndexType>(remove_idx);
                    }
                    dense_data.pop_back();
                    dense_indices.pop_back();

                    sparse[static_cast<size_t>(id)] = null_index;
                    free_reverse[static_cast<size_t>(id)] = static_cast<IndexType>(free_ids.size());
                    free_ids.push_back(id);
                    ++deleted_count;
                }
                else
                {
                    ++i;
                }
            }
            return deleted_count;
        }

        void DeleteAll() noexcept
        {
            dense_data.clear();
            dense_indices.clear();
            sparse.clear();
            free_ids.clear();
            free_reverse.clear();
        }

        [[nodiscard]] size_t Size() const noexcept { return dense_data.size(); }
        [[nodiscard]] bool Empty() const noexcept { return dense_data.empty(); }

        DataType *Data() noexcept { return dense_data.data(); }
        const DataType *Data() const noexcept { return dense_data.data(); }

        auto begin() noexcept { return dense_data.begin(); }
        auto end() noexcept { return dense_data.end(); }
        auto begin() const noexcept { return dense_data.cbegin(); }
        auto end() const noexcept { return dense_data.cend(); }
        auto cbegin() const noexcept { return dense_data.cbegin(); }
        auto cend() const noexcept { return dense_data.cend(); }

    private:
        [[nodiscard]] static const DataType *Probe(
            const std::vector<IndexType> &sp,
            const std::vector<IndexType> &di,
            const std::vector<DataType> &dd,
            IndexType id) noexcept
        {
            if constexpr (std::is_signed_v<IndexType>)
            {
                if (id < 0) return nullptr;
            }
            size_t u = static_cast<size_t>(id);
            if (u >= sp.size()) return nullptr;
            IndexType d = sp[u];
            if (d == null_index) return nullptr;
            size_t ud = static_cast<size_t>(d);
            if (ud >= di.size() || di[ud] != id) return nullptr;
            return &dd[ud];
        }

        [[nodiscard]] DataType *TryGetImpl(IndexType id) noexcept
        {
            return const_cast<DataType *>(Probe(sparse, dense_indices, dense_data, id));
        }

        [[nodiscard]] const DataType *TryGetImpl(IndexType id) const noexcept
        {
            return Probe(sparse, dense_indices, dense_data, id);
        }

        void RemoveFromFreeSet(IndexType id) noexcept
        {
            size_t u_id = static_cast<size_t>(id);
            IndexType pos = free_reverse[u_id];
            if (pos == null_index) return;

            free_reverse[u_id] = null_index;
            size_t u_pos = static_cast<size_t>(pos);

            if (u_pos != free_ids.size() - 1)
            {
                IndexType swapped_id = free_ids.back();
                free_ids[u_pos] = swapped_id;
                free_reverse[static_cast<size_t>(swapped_id)] = pos;
            }
            free_ids.pop_back();
        }
    };

}