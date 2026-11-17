#pragma once
#include "RefControlBlock.hpp"
#include <vector>

namespace RandomEngine::Core::Memory
{
    // RAII 域：图对象当成员持有即可，析构自动收环。
    // parent 可选：子域析构把存活块移交给父域，嵌套子图的环由共同祖先收。
    class RefDomain
    {
    public:
        explicit RefDomain(RefDomain* parent = nullptr) noexcept : parent_(parent) {}
        RefDomain(const RefDomain&) = delete;
        RefDomain& operator=(const RefDomain&) = delete;
        ~RefDomain() { Reclaim(); }

        void Register(RefControlBlock* b) noexcept {
            b->domain = this; b->next = head_; head_ = b;
        }

    private:
        void Reclaim() noexcept {
            // 1. 本域活块快照；strong 已隐含全部外部根，不需要枚举 RootSet
            std::vector<RefControlBlock*> S;
            for (auto* b = head_; b; b = b->next) {
                if (b->Closed() || b->Reaped()) continue;
                b->in_S = true; b->mark = 0;
                b->gc_refs = b->StrongCount();
                S.push_back(b);
            }

            // 2. 减本域内部入边；跨域块 in_S=false，视为外部根
            RefVisitor sub{nullptr, [](void*, RefControlBlock* to) noexcept {
                if (to && to->in_S && to->gc_refs) --to->gc_refs;
            }};
            for (auto* b : S) b->Trace(sub);

            // 3. gc_refs > 0 = 有外部根 → 传播 reachable
            std::vector<RefControlBlock*> stack;
            for (auto* b : S)
                if (b->gc_refs > 0 && !b->mark) { b->mark = 1; stack.push_back(b); }
            RefVisitor walk{&stack, [](void* ctx, RefControlBlock* to) noexcept {
                auto* st = static_cast<std::vector<RefControlBlock*>*>(ctx);
                if (to && to->in_S && !to->mark) { to->mark = 1; st->push_back(to); }
            }};
            while (!stack.empty()) { auto* b = stack.back(); stack.pop_back(); b->Trace(walk); }

            // 4. 无根环：整组先 Close 再逐个销毁（避免级联时对方尚未关闭）
            for (auto* b : S) if (!b->mark && b->gc_refs == 0) b->Close();
            for (auto* b : S) if (!b->mark && b->gc_refs == 0) b->TryDestroyNow();

            // 5. 只要还有一个对象活着，就不能删任何块：
            //    环里后析构的对象还会 Release 到先删的块上
            bool any_alive = false;
            for (auto* b = head_; b; b = b->next)
                if (!b->Reaped()) { any_alive = true; break; }

            RefControlBlock* cur = head_;
            head_ = nullptr;
            while (cur) {
                RefControlBlock* next = cur->next;
                cur->next = nullptr; cur->in_S = false;
                if (!any_alive && cur->Reaped() && cur->StrongCount() == 0 && !cur->HasReader()) {
                    delete cur;                       // 整域回收干净
                } else if (parent_) {
                    parent_->Register(cur);           // 移交给父域
                } else {
                    cur->orphaned = true;             // 域外仍有 RefPtr：最后一次 Release/Leave 自删
                    cur->domain   = nullptr;
                }
                cur = next;
            }
        }

        RefDomain*       parent_ = nullptr;
        RefControlBlock* head_   = nullptr;
    };
}