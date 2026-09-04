#include <cstdint>
#include <vector>
#include <cassert>
#include <stdexcept>
#include <span>

namespace RandomEngine::Core::Containers
{

    // 稀疏集
    template <typename DataType, typename IndexType = uint64_t>
    struct SparseSet
    {
    private:
        static_assert(std::is_integral_v<IndexType>, "IndexType must be an integral type!");

        static constexpr IndexType null_index = static_cast<IndexType>(-1);

        std::vector<IndexType> sparse;        // ID -> dense 下标 (空闲时作为 next_free_id)
        std::vector<IndexType> dense_indices; // dense 下标 -> ID (Swap-and-Pop 映射)
        std::vector<DataType> dense_data;     // 纯粹连续的 DataType 内存块
        IndexType first_free{null_index};     // 空闲链表头节点

    public:
        // 构造析构
        SparseSet() = default;

        explicit SparseSet(IndexType init_size)
        {
            if constexpr (std::is_signed_v<IndexType>)
            {
                assert(init_size >= 0 && "Initial capacity cannot be negative!");
            }

            size_t cap = static_cast<size_t>(init_size);

            // 仅分配物理内存，不改变逻辑大小（size() 依然为 0）
            sparse.reserve(cap);
            dense_indices.reserve(cap);
            dense_data.reserve(cap);
        }

        // id分配 查询
        IndexType AllocateID()
        {
            if (first_free != null_index)
            {
                IndexType id = first_free;
                first_free = sparse[static_cast<size_t>(id)];
                sparse[static_cast<size_t>(id)] = null_index; // 重置为干净状态
                return id;
            }
            IndexType id = static_cast<IndexType>(sparse.size());
            sparse.push_back(null_index);
            return id;
        }

        [[nodiscard]] bool Contains(IndexType id) const noexcept
        {
            if (id >= sparse.size())
                return false;

            IndexType d_idx = sparse[static_cast<size_t>(id)];
            if (d_idx == null_index)
                return false;

            size_t u_d_idx = static_cast<size_t>(d_idx);
            return u_d_idx < dense_indices.size() && dense_indices[u_d_idx] == id;
        }

        // 插入
        void Insert(IndexType id, const DataType &data)
        {
            if (id == null_index) {
                throw std::invalid_argument("Reserved null_index cannot be used as a valid ID!");
            }

            // 结合 TryGetImpl 一次寻址，若存在直接覆盖更新
            if (auto *ptr = TryGetImpl(id)) {
                *ptr = data;
                return;
            }

            size_t u_id = static_cast<size_t>(id);

            if (u_id >= sparse.size()) {
                sparse.resize(u_id + 1, null_index);
            } else {
                UnlinkFromFreeList(id); // ✅ 无条件安全解链
            }

            dense_data.push_back(data);
            dense_indices.push_back(id);
            sparse[u_id] = static_cast<IndexType>(dense_data.size() - 1);
        }

        void Insert(IndexType id, DataType &&data)
        {
            if (id == null_index) {
                throw std::invalid_argument("Reserved null_index cannot be used as a valid ID!");
            }

            // 结合 TryGetImpl 一次寻址，若存在直接移动覆盖
            if (auto *ptr = TryGetImpl(id)) {
                *ptr = std::move(data);
                return;
            }

            size_t u_id = static_cast<size_t>(id);

            if (u_id >= sparse.size()) {
                sparse.resize(u_id + 1, null_index);
            } else {
                UnlinkFromFreeList(id); // ✅ 无条件安全解链
            }

            dense_data.push_back(std::move(data));
            dense_indices.push_back(id);
            sparse[u_id] = static_cast<IndexType>(dense_data.size() - 1);
        }

        IndexType Insert(const DataType &data)
        {
            IndexType id = AllocateID(); // AllocateID 已将该 id 弹出空闲链表
            size_t u_id = static_cast<size_t>(id);

            sparse[u_id] = static_cast<IndexType>(dense_data.size());
            dense_indices.push_back(id);
            dense_data.push_back(data);

            return id;
        }

        IndexType Insert(DataType &&data)
        {
            IndexType id = AllocateID();
            size_t u_id = static_cast<size_t>(id);

            sparse[u_id] = static_cast<IndexType>(dense_data.size());
            dense_indices.push_back(id);
            dense_data.push_back(std::move(data));

            return id;
        }

        // 获取
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
            if (!Contains(id))
                return nullptr;
            return &dense_data[static_cast<size_t>(sparse[static_cast<size_t>(id)])];
        }

        const DataType *GetPtr(IndexType id) const noexcept
        {
            if (!Contains(id))
                return nullptr;
            return &dense_data[static_cast<size_t>(sparse[static_cast<size_t>(id)])];
        }

        std::vector<DataType> Get(const std::vector<IndexType> &ids)
        {
            std::vector<DataType> result;
            result.reserve(ids.size());
            for (const auto &id : ids)
            {
                if (Contains(id))
                {
                    result.push_back(dense_data[static_cast<size_t>(sparse[static_cast<size_t>(id)])]);
                }
            }
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

            // 获取 dense_data 的内存边界
            const DataType *start = dense_data.data();
            const DataType *end = start + dense_data.size();

            for (const auto &item : datas)
            {
                const DataType *ptr = &item;

                // 【Fast Path】O(1) 地址计算：检测对象地址是否属于 dense_data 堆空间
                if (ptr >= start && ptr < end)
                {
                    size_t dense_idx = static_cast<size_t>(ptr - start);
                    result.push_back(dense_indices[dense_idx]);
                }
                // 【Fallback Path】O(N) 值比对：对象是外部副本，进行遍历匹配
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
            result.reserve(data_ptrs.size()); // 提前预分配，避免多次 realloc

            for (const auto *ptr : data_ptrs)
            {
                result.push_back(GetIndex(ptr)); // 复用 O(1) 的指针偏移算术
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

                // Fast Path: O(1) 地址区间与对齐校验
                if (ptr >= start && ptr < end &&
                    (reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(start)) % sizeof(DataType) == 0)
                {
                    size_t dense_idx = static_cast<size_t>(ptr - start);
                    result.push_back(dense_indices[dense_idx]);
                }
                // Fallback Path: O(N) 值比对
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

        /// @brief 获取所有数据/行为的连续内存视图 (只读)
        [[nodiscard]] std::span<const DataType> GetAll() const noexcept
        {
            return dense_data;
        }

        /// @brief 获取所有已激活实体的 ID 列表视图 (与 dense_data 索引一一对应)
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

        // 删除
        bool Delete(IndexType id) noexcept
        {
            if (!Contains(id))
                return false;

            size_t u_id = static_cast<size_t>(id);
            size_t remove_idx = static_cast<size_t>(sparse[u_id]);
            size_t last_idx = dense_data.size() - 1;

            // Swap-and-Pop 保持 dense 内存 100% 连续
            if (remove_idx != last_idx)
            {
                dense_data[remove_idx] = std::move(dense_data.back());
                dense_indices[remove_idx] = dense_indices.back();
                sparse[static_cast<size_t>(dense_indices[remove_idx])] = static_cast<IndexType>(remove_idx);
            }

            dense_data.pop_back();
            dense_indices.pop_back();

            // 头插法回收 ID 到 sparse 构成的嵌入式链表中
            sparse[u_id] = first_free;
            first_free = id;

            return true;
        }

        size_t Delete(std::span<const IndexType> ids) noexcept
        {
            size_t deleted_count = 0;
            for (IndexType id : ids)
            {
                if (Delete(id))
                { // 复用已经写好的 O(1) Delete(id)
                    ++deleted_count;
                }
            }
            return deleted_count;
        }

        // 语法糖：支持直接大括号调用 set.DeleteAll({ id1, id2, id3 });
        size_t Delete(std::initializer_list<IndexType> ids) noexcept
        {
            return Delete(std::span<const IndexType>(ids.begin(), ids.size()));
        }

        bool Delete(const DataType *ptr) noexcept
        {
            if (!ptr || dense_data.empty())
                return false;

            // 校验地址是否在 dense_data 连续内存块内，且必须是对象首地址
            const DataType *start = dense_data.data();
            const DataType *end = start + dense_data.size();

            if (ptr >= start && ptr < end)
            {
                uintptr_t offset = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(start);
                if (offset % sizeof(DataType) == 0)
                { // 确认是对象首地址
                    size_t dense_idx = static_cast<size_t>(ptr - start);
                    return Delete(dense_indices[dense_idx]); // 完美转交 O(1) 的 Delete(id)
                }
            }

            return false;
        }

        bool Delete(const DataType &object) noexcept
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

                    // ✅ 原地 Swap-and-Pop，无需二次寻址
                    if (remove_idx != last_idx)
                    {
                        dense_data[remove_idx] = std::move(dense_data.back());
                        dense_indices[remove_idx] = dense_indices.back();
                        sparse[static_cast<size_t>(dense_indices[remove_idx])] = static_cast<IndexType>(remove_idx);
                    }
                    dense_data.pop_back();
                    dense_indices.pop_back();

                    sparse[static_cast<size_t>(id)] = first_free;
                    first_free = id;
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
            sparse.clear(); // ✅ 直接清空 sparse，重置整体空间
            first_free = null_index;
        }

        // 7. 迭代器与容器属性
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
        void UnlinkFromFreeList(IndexType id) noexcept
        {
            if (first_free == null_index)
                return;
            if (first_free == id)
            {
                first_free = sparse[static_cast<size_t>(id)];
                return;
            }
            IndexType curr = first_free;
            while (curr != null_index)
            {
                size_t u_curr = static_cast<size_t>(curr);
                IndexType next = sparse[u_curr];
                if (next == id)
                {
                    sparse[u_curr] = sparse[static_cast<size_t>(id)];
                    break;
                }
                curr = next;
            }
        }

        [[nodiscard]] DataType *TryGetImpl(IndexType id) noexcept
        {
            if constexpr (std::is_signed_v<IndexType>)
            {
                if (id < 0)
                    return nullptr;
            }
            size_t u_id = static_cast<size_t>(id);
            if (u_id >= sparse.size())
                return nullptr;

            IndexType d_idx = sparse[u_id];
            if (d_idx == null_index)
                return nullptr;

            size_t u_d_idx = static_cast<size_t>(d_idx);
            if (u_d_idx >= dense_indices.size() || dense_indices[u_d_idx] != id)
                return nullptr;

            return &dense_data[u_d_idx];
        }
    };

}