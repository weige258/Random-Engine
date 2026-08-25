#pragma once

#include "Core/Behaviors/BaseBehavior/BaseBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include "Systems/ISystem.hpp"
#include <memory>
#include <vector>
#include <utility>

namespace RandEngine::Systems::ResourceSystems
{

    class BehaviorSystem : Systems::ISystem
    {
    private:
        std::vector<Core::Memory::MasterPtr<Core::Behaviors::BaseBehavior>> all_behaviors;

        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>> behaviors_should_add;
        std::vector<Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>> behaviors_should_delete;

    public:
        template <typename T>
        void AddBehavior(Core::Memory::MasterPtr<T> behavior)
        {
            if (!behavior)
                return;

            behaviors_should_add.push_back(Core::Memory::ObserverPtr<Core::Behaviors::BaseBehavior>(behavior));

            all_behaviors.push_back(std::move(behavior));
        }

        template <typename T>
        void DeleteBehavior(Core::Memory::ObserverPtr<T> behavior)
        {
            if (!behavior)
                return;

            behaviors_should_delete.push_back(behavior);

            std::erase_if(all_behaviors, [&](const auto &master)
                          { return master.Get() == behavior.Get(); });
        }

        template <typename T>
        std::pair<std::vector<Core::Memory::ObserverPtr<T>>,
                  std::vector<Core::Memory::ObserverPtr<T>>>
        RemoveBehaviorToRuntimeSystem()
        {
            std::vector<Core::Memory::ObserverPtr<T>> matched_add;
            std::vector<Core::Memory::ObserverPtr<T>> matched_delete;

            if (behaviors_should_add.empty() && behaviors_should_delete.empty())
            {
                return {std::move(matched_add), std::move(matched_delete)};
            }

            // 抽取匹配接口的双指针高效提取 Lambda
            auto extract_matching = [&](auto &src_vec, auto &out_vec)
            {
                if (src_vec.empty())
                    return;

                size_t write_idx = 0;
                const size_t size = src_vec.size();

                for (size_t read_idx = 0; read_idx < size; ++read_idx)
                {
                    auto &item = src_vec[read_idx];
                    if (item)
                    {
                        // 使用标准的 dynamic_observer_cast 进行安全的 RTTI 检查与接口转型
                        auto casted = Core::Memory::dynamic_observer_cast<T>(item);
                        if (casted)
                        {
                            out_vec.push_back(std::move(casted));
                        }
                        else
                        {
                            // 未匹配上 T 接口的 Behavior 保留留在原队列中
                            if (write_idx != read_idx)
                            {
                                src_vec[write_idx] = std::move(item);
                            }
                            ++write_idx;
                        }
                    }
                }
                src_vec.resize(write_idx);
            };

            extract_matching(behaviors_should_add, matched_add);
            extract_matching(behaviors_should_delete, matched_delete);

            return {std::move(matched_add), std::move(matched_delete)};
        }

        void Init(System &system) {};

        void Run(System &system) {

        };

        void Destroy() {};
    };
}
