#pragma once
#include <vector>
#include <array>
#include "BaseJob.hpp"

namespace RandomEngine::Core::Jobs::Job
{
    template <typename Job, std::size_t ChunkSize = 0>
        requires(IsBaseJob<Job>)
    struct BaseChunkJob
    {
        static constexpr bool DynamicChunk = (ChunkSize == 0);

    private:
        std::conditional_t<DynamicChunk, std::vector<Job>, std::array<Job, ChunkSize>> m_jobs;

    public:
        template <typename SlotType, typename... Args>
            requires(Detail::SlotCallable<SlotType, Args...>)
        void Execute(Args&&... args)
        {
            for (auto& job : m_jobs)
            {
                if (job)
                    job.template Execute<SlotType>(std::forward<Args>(args)...);
            }
        }

        template <typename... Args>
            requires(std::tuple_size_v<typename Job::SlotTuple> == 1 &&
                     Detail::SlotCallable<std::tuple_element_t<0, typename Job::SlotTuple>, Args...>)
        void Execute(Args&&... args)
        {
            Execute<std::tuple_element_t<0, typename Job::SlotTuple>>(std::forward<Args>(args)...);
        }
    };
}