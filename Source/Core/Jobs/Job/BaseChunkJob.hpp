#pragma once
#include <vector>
#include <array>
#include "BaseJob.hpp"


namespace RandomEngine::Core::Jobs::Job{

    template <typename Job, std::size_t ChunkSize=0>
    struct BaseChunkJob
    {
        static constexpr bool DynamicChunk = (ChunkSize == 0);

        std::conditional_t<DynamicChunk,
        std::vector<Job>,
        std::array<Job, ChunkSize>> m_jobs;

        virtual void Execute() {
            for (auto& job : m_jobs) {
                job.Execute();
            }
        };

    };
}