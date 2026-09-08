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
        using ExecuteArgsTuple = typename Job::ExecuteArgsTuple;

    private:
        std::conditional_t<DynamicChunk, std::vector<Job>, std::array<Job, ChunkSize>> m_jobs;

    public:
        template <typename... Args>
        void Execute(Args&&... args)
        {
            for (auto& job : m_jobs)
            {
                if (job)
                    job.Execute(std::forward<Args>(args)...);
            }
        }

        template <typename... Args>
        void Execute(Args&&... args)
        {
            static_assert(
                std::is_same_v<ExecuteArgsTuple, std::tuple<std::remove_cvref_t<Args>...>>,
                "Execute args must match Job::ExecuteArgsTuple");

            for (auto& job : m_jobs)
            {
                if (job)
                    job.Execute(std::forward<Args>(args)...);
            }
        }
    };
}