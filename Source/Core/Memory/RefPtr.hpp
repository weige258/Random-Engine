#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace RandomEngine::Core::Memory
{
    struct RefControlBlock;

    struct RefVisitor
    {
        void* ctx = nullptr;
        void (*edge)(void*, RefControlBlock*) noexcept = nullptr;
        void Edge(RefControlBlock* to) const noexcept { if (edge && to) edge(ctx, to); }
    };

    // 叶子类型默认没有出边；需要参与环回收的类型，在自己命名空间写一次同名重载：
    //   inline void TraceEdges(const MyNode& n, RefVisitor& v) noexcept {
    //       if (n.next) v.Edge(n.next.Block());
    //   }
    // 跟写析构函数一样，是类型定义的一部分，不是运行时调用。
    template <class T>
    inline void TraceEdges(const T&, RefVisitor&) noexcept {}

    // 实现内部状态（thread_local，使用方完全不可见）：只用于阻止回收过程中的
    // 重入与提前释放控制块。不是外部结构，不需要任何人创建或调用。
    inline thread_local bool t_collecting = false;

    // 外置控制块：bit63 Closing / bits32..62 Strong / bits0..31 Reader
    struct RefControlBlock
    {
        static constexpr std::uint64_t ClosingBit = 1ull << 63;
        static constexpr std::uint64_t StrongUnit = 1ull << 32;
        static constexpr std::uint64_t StrongMask = 0x7FFFFFFFull << 32;
        static constexpr std::uint64_t ReaderMask = 0xFFFFFFFFull;

        std::atomic<std::uint64_t> state{0};
        std::atomic<bool>          reaped{false};

        void* target  = nullptr;
        void (*destroy)(void*) noexcept = nullptr;
        void (*trace)(void*, RefVisitor&) noexcept = nullptr;

        // 回收临时字段：只在 CollectFrom() 内读写，侵入链零堆分配
        std::uint32_t    gc_refs = 0;
        RefControlBlock* gc_next = nullptr;
        RefControlBlock* gc_link = nullptr;
        bool             in_S = false;
        bool             mark = false;

        std::uint32_t StrongCount() const noexcept
        { return std::uint32_t((state.load(std::memory_order_acquire) & StrongMask) >> 32); }
        bool Closed()    const noexcept { return (state.load(std::memory_order_acquire) & ClosingBit) != 0; }
        bool Reaped()    const noexcept { return reaped.load(std::memory_order_acquire); }
        bool HasReader() const noexcept { return (state.load(std::memory_order_acquire) & ReaderMask) != 0; }

        void AddRef() noexcept { state.fetch_add(StrongUnit, std::memory_order_relaxed); }

        void Release() noexcept
        {
            const std::uint64_t old = state.fetch_sub(StrongUnit, std::memory_order_acq_rel);
            const std::uint32_t before = std::uint32_t((old & StrongMask) >> 32);
            if (before <= 1)                       // 归零（或防御下溢）
            {
                Close();
                TryDestroyNow();
                MaybeFreeBlock();
                return;
            }

            // 叶子快速路径：没有任何出边就不可能成环，直接返回。
            // 全程只做原子计数 + 只读 trace/target，不碰任何 GC 临时字段，
            // 因此多线程并发 ref/unref 叶子是安全的（TSAN 验证）。
            bool has_out = false;
            RefVisitor probe{&has_out, [](void* c, RefControlBlock*) noexcept
            { *static_cast<bool*>(c) = true; }};
            Trace(probe);
            if (!has_out) return;

            CollectFrom();                         // 降而未零且真有出边：可能是环
            // 注意：CollectFrom 可能 delete this，此后绝不再访问任何成员
        }

        // O(1) 关闭入口，立即返回，不等任何人
        void Close() noexcept { state.fetch_or(ClosingBit, std::memory_order_acq_rel); }

        bool TryEnter() noexcept
        {
            std::uint64_t s = state.load(std::memory_order_acquire);
            for (;;)
            {
                if (s & ClosingBit) return false;
                if ((s & ReaderMask) == ReaderMask) return false;
                if (state.compare_exchange_weak(s, s + 1,
                        std::memory_order_acquire, std::memory_order_relaxed)) return true;
            }
        }

        void Leave() noexcept
        {
            state.fetch_sub(1, std::memory_order_acq_rel);
            TryDestroyNow();
            MaybeFreeBlock();
        }

        // 只销毁对象，不释放控制块；reader > 0 时直接返回（不打断执行者）
        void TryDestroyNow() noexcept
        {
            const std::uint64_t s = state.load(std::memory_order_acquire);
            if (!(s & ClosingBit) || (s & ReaderMask) != 0) return;
            if (reaped.exchange(true, std::memory_order_acq_rel)) return;
            void* p = target;
            target = nullptr;
            if (destroy) destroy(p);
        }

        void MaybeFreeBlock() noexcept
        {
            if (t_collecting) return;             // 回收末尾统一释放
            if (Closed() && Reaped() && StrongCount() == 0 && !HasReader()) delete this;
        }

        void Trace(RefVisitor& v) noexcept { if (trace && target) trace(target, v); }

    private:
        // 局部试删除：从「刚 Release 的块」沿出边走一圈。
        // strong 本身已隐含全部外部根，不需要全表、不需要 RootSet、不需要外部触发。
        // 约束：带出边对象的构图/释放是单线程事务（编辑器、Shader 重编译本就如此）；
        //       叶子类型走上面的快速路径，不受此约束。
        void CollectFrom() noexcept
        {
            if (t_collecting || Closed() || Reaped()) return;   // 防重入
            t_collecting = true;

            struct Build { RefControlBlock* head = nullptr; RefControlBlock* tail = nullptr; };

            // 1. 候选集 S = 从本块沿出边的可达闭包
            Build B;
            in_S = true; mark = false; gc_refs = StrongCount();
            gc_next = nullptr; gc_link = nullptr;
            B.head = B.tail = this;

            RefVisitor grow{&B, [](void* c, RefControlBlock* to) noexcept {
                auto* b = static_cast<Build*>(c);
                if (!to || to->in_S || to->Closed() || to->Reaped()) return;
                to->in_S = true; to->mark = false;
                to->gc_refs = to->StrongCount();
                to->gc_next = nullptr; to->gc_link = nullptr;
                if (b->tail) b->tail->gc_next = to; else b->head = to;
                b->tail = to;
            }};
            for (auto* b = B.head; b; b = b->gc_next) b->Trace(grow);

            // 2. 减去 S 内部的入边
            RefVisitor sub{nullptr, [](void*, RefControlBlock* to) noexcept {
                if (to && to->in_S && to->gc_refs) --to->gc_refs;
            }};
            for (auto* b = B.head; b; b = b->gc_next) b->Trace(sub);

            // 3. gc_refs > 0 ⇒ 有 S 外部的 RefPtr 指着它 ⇒ 传播 reachable
            RefControlBlock* st = nullptr;
            for (auto* b = B.head; b; b = b->gc_next)
                if (b->gc_refs > 0 && !b->mark) { b->mark = true; b->gc_link = st; st = b; }
            RefVisitor walk{&st, [](void* c, RefControlBlock* to) noexcept {
                auto** s = static_cast<RefControlBlock**>(c);
                if (to && to->in_S && !to->mark)
                { to->mark = true; to->gc_link = *s; *s = to; }
            }};
            while (st) { auto* b = st; st = b->gc_link; b->Trace(walk); }

            // 4. 无根子图：整组先 Close，再逐个销毁（避免级联时对方尚未关闭）
            for (auto* b = B.head; b; b = b->gc_next)
                if (!b->mark && b->gc_refs == 0) b->Close();
            for (auto* b = B.head; b; b = b->gc_next)
                if (!b->mark && b->gc_refs == 0) b->TryDestroyNow();

            // 5. 复位 GC 字段，统一释放已销毁的块
            auto* cur = B.head;
            while (cur)
            {
                auto* next = cur->gc_next;
                cur->in_S = false; cur->mark = false;
                cur->gc_next = nullptr; cur->gc_link = nullptr;
                if (cur->Reaped() && cur->StrongCount() == 0 && !cur->HasReader()) delete cur;
                cur = next;
            }
            t_collecting = false;   // 不访问 this；delete this 之后执行也安全
        }
    };

    template <class T> struct RefPtr;

    // 安全调用：move-only，见 ClosingBit 直接给空，无等待边 ⇒ 无 ABBA
    template <class T>
    struct ScopedRef
    {
    private:
        T* m_ptr = nullptr; RefControlBlock* m_block = nullptr;
        friend struct RefPtr<T>;
        ScopedRef(T* p, RefControlBlock* b) noexcept : m_ptr(p), m_block(b) {}
    public:
        ScopedRef() noexcept = default;
        ScopedRef(const ScopedRef&) = delete;
        ScopedRef& operator=(const ScopedRef&) = delete;
        ScopedRef(ScopedRef&& o) noexcept : m_ptr(o.m_ptr), m_block(o.m_block)
        { o.m_ptr = nullptr; o.m_block = nullptr; }
        ScopedRef& operator=(ScopedRef&& o) noexcept
        {
            if (this != &o)
            {
                if (m_block) m_block->Leave();
                m_ptr = o.m_ptr; m_block = o.m_block;
                o.m_ptr = nullptr; o.m_block = nullptr;
            }
            return *this;
        }
        ~ScopedRef() { if (m_block) m_block->Leave(); }
        explicit operator bool() const noexcept { return m_ptr != nullptr; }
        T* Get()        const noexcept { return m_ptr; }
        T* operator->() const noexcept { return m_ptr; }
        T& operator*()  const noexcept { return *m_ptr; }
    };

    template <class T>
    struct RefPtr
    {
    private:
        T* m_ptr = nullptr;
        RefControlBlock* m_block = nullptr;
        template <class U> friend struct RefPtr;
    public:
        using element_type = T;
        constexpr RefPtr() noexcept = default;
        constexpr RefPtr(std::nullptr_t) noexcept {}

        static RefPtr FromBlock(T* p, RefControlBlock* b) noexcept
        { RefPtr r; r.m_ptr = p; r.m_block = b; if (b) b->AddRef(); return r; }

        RefPtr(const RefPtr& o) noexcept : m_ptr(o.m_ptr), m_block(o.m_block)
        { if (m_block) m_block->AddRef(); }
        template <class U> requires std::is_convertible_v<U*, T*>
        RefPtr(const RefPtr<U>& o) noexcept : m_ptr(o.m_ptr), m_block(o.m_block)
        { if (m_block) m_block->AddRef(); }

        RefPtr(RefPtr&& o) noexcept : m_ptr(o.m_ptr), m_block(o.m_block)
        { o.m_ptr = nullptr; o.m_block = nullptr; }
        template <class U> requires std::is_convertible_v<U*, T*>
        RefPtr(RefPtr<U>&& o) noexcept : m_ptr(o.m_ptr), m_block(o.m_block)
        { o.m_ptr = nullptr; o.m_block = nullptr; }

        ~RefPtr() { Reset(); }

        RefPtr& operator=(const RefPtr& o) noexcept
        { if (this != &o) RefPtr(o).Swap(*this); return *this; }
        RefPtr& operator=(RefPtr&& o) noexcept
        { if (this != &o) RefPtr(std::move(o)).Swap(*this); return *this; }
        RefPtr& operator=(std::nullptr_t) noexcept { Reset(); return *this; }

        // 先清空再 Release：析构重入时看到的是空
        void Reset() noexcept
        {
            RefControlBlock* b = m_block;
            m_ptr = nullptr; m_block = nullptr;
            if (b) b->Release();
        }
        void Swap(RefPtr& o) noexcept
        { std::swap(m_ptr, o.m_ptr); std::swap(m_block, o.m_block); }

        T* Get()        const noexcept { return m_ptr; }
        T* operator->() const noexcept { return m_ptr; }
        T& operator*()  const noexcept { return *m_ptr; }
        explicit operator bool() const noexcept { return m_ptr != nullptr; }
        RefControlBlock* Block() const noexcept { return m_block; }

        template <class U> bool operator==(const RefPtr<U>& o) const noexcept
        { return m_ptr == o.Get(); }
        bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }

        template <class U> RefPtr<U> StaticCast() const noexcept
        { return RefPtr<U>::FromBlock(static_cast<U*>(m_ptr), m_block); }
        template <class U> RefPtr<U> DynamicCast() const noexcept
        {
            U* q = dynamic_cast<U*>(m_ptr);
            return q ? RefPtr<U>::FromBlock(q, m_block) : RefPtr<U>{};
        }

        ScopedRef<T> Lock() const noexcept
        {
            if (!m_block || !m_block->TryEnter()) return {};
            return ScopedRef<T>(m_ptr, m_block);
        }
    };

    // 唯一创建入口：零额外参数，使用方零外部调用
    template <class T, class... Args>
    RefPtr<T> MakeRef(Args&&... args)
    {
        auto* b = new RefControlBlock;
        try { b->target = new T(std::forward<Args>(args)...); }
        catch (...) { delete b; throw; }
        b->destroy = [](void* q) noexcept { delete static_cast<T*>(q); };
        b->trace   = [](void* q, RefVisitor& v) noexcept
        { TraceEdges(*static_cast<T*>(q), v); };   // ADL
        return RefPtr<T>::FromBlock(static_cast<T*>(b->target), b);
    }
}

template <class T>
struct std::hash<RandomEngine::Core::Memory::RefPtr<T>>
{
    std::size_t operator()(const RandomEngine::Core::Memory::RefPtr<T>& p) const noexcept
    { return std::hash<T*>{}(p.Get()); }
};
