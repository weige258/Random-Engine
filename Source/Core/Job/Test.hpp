#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <cstdint>
#include <random>
#include <thread>
#include <memory>
#include <sstream>
#include <cmath>

#include "AffinityJobWorker.hpp"
#include "SharedJobWorker.hpp"
#include "Core/Math/Math.hpp"   // Vec3f, Length, Normalize

namespace RandEngine::Core::Job
{
    // -----------------------------------------------------------------------------
    // 1. 游戏实体任务（可默认构造，不可拷贝/移动）
    // -----------------------------------------------------------------------------
    struct EntityTask
    {
        static constexpr float DT = 0.016f;
        static constexpr float GRAVITY = 9.81f;
        static constexpr float DRAG = 0.999f;
        static constexpr float ATTRACTION = 0.5f;

        RandEngine::Core::Math::Vec3f position;
        RandEngine::Core::Math::Vec3f velocity;
        float mass = 1.0f;
        std::atomic<uint64_t> run_count{0};

        // 随机化实体（重置 run_count）
        void Randomize(std::mt19937& rng)
        {
            std::uniform_real_distribution<float> pos_dist(-100.0f, 100.0f);
            std::uniform_real_distribution<float> vel_dist(-5.0f, 5.0f);
            std::uniform_real_distribution<float> mass_dist(0.5f, 2.0f);

            position = {pos_dist(rng), pos_dist(rng), pos_dist(rng)};
            velocity = {vel_dist(rng), vel_dist(rng), vel_dist(rng)};
            mass = mass_dist(rng);
            run_count = 0;   // 重置统计
        }

        // 执行一次更新（物理+AI 模拟）
        void Execute()
        {
            // 重力
            velocity += {0.0f, -GRAVITY * DT, 0.0f};

            // 向原点吸引力
            auto dir = -position;
            float len = RandEngine::Core::Math::Length(dir);
            if (len > 0.001f)
            {
                auto force = (dir / len) * ATTRACTION;
                velocity += force * DT;
            }

            // 空气阻力
            velocity *= DRAG;

            // 更新位置
            position += velocity * DT;

            // 边界约束（反弹）
            constexpr float BOUNDARY = 150.0f;
            for (int i = 0; i < 3; ++i)
            {
                if (position[i] > BOUNDARY)
                {
                    position[i] = BOUNDARY;
                    velocity[i] = -velocity[i] * 0.5f;
                }
                else if (position[i] < -BOUNDARY)
                {
                    position[i] = -BOUNDARY;
                    velocity[i] = -velocity[i] * 0.5f;
                }
            }

            run_count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    // -----------------------------------------------------------------------------
    // 2. 派生 Worker 类
    // -----------------------------------------------------------------------------
    class EntityAffinityWorker : public AffinityJobWorker<EntityTask>
    {
    protected:
        void ExecuteTask(EntityTask* task) override
        {
            task->Execute();
        }
    };

    class EntitySharedWorker : public SharedJobWorker<EntityTask>
    {
    public:
        EntitySharedWorker(size_t capacity = 4096) : SharedJobWorker<EntityTask>(capacity) {}

    protected:
        void ExecuteTask(EntityTask* task) override
        {
            task->Execute();
        }
    };

    // -----------------------------------------------------------------------------
    // 3. 性能测试函数
    // -----------------------------------------------------------------------------
    inline void RunWorkerBenchmark()
    {
        constexpr size_t ENTITY_COUNT = 10000;
        constexpr auto TEST_DURATION = std::chrono::milliseconds(3000);
        const std::vector<size_t> THREAD_CONFIGS = {1, 2, 4, 8, 16,24,32,48,60};

        std::cout << "========================================================================\n";
        std::cout << "  Game Engine Worker Benchmark (10,000 entities, physics + AI)\n";
        std::cout << "  Duration: 3000ms per test\n";
        std::cout << "========================================================================\n\n";

        std::cout << std::left
                  << std::setw(12) << "Threads"
                  << std::setw(28) << "AffinityWorker (Ops/sec)"
                  << std::setw(28) << "SharedJobWorker (Ops/sec)"
                  << std::setw(14) << "Diff %"
                  << "\n------------------------------------------------------------------------\n";

        std::random_device rd;
        std::mt19937 rng(rd());

        // 汇总统计（跨线程配置累计）
        uint64_t sum_affinity_ops = 0;
        uint64_t sum_shared_ops = 0;
        double max_gain_percent = 0.0;
        size_t max_gain_threads = 0;
        bool any_result = false;

        for (size_t threads : THREAD_CONFIGS)
        {
            // -------------------------------------------------------------
            // 测试 1: AffinityLoopWorker
            // -------------------------------------------------------------
            std::vector<EntityTask> affinity_entities(ENTITY_COUNT);   // 默认构造
            for (auto& e : affinity_entities)
                e.Randomize(rng);

            EntityAffinityWorker affinity_worker;
            affinity_worker.Start(threads);

            for (size_t i = 0; i < ENTITY_COUNT; ++i)
                affinity_worker.PushTask(&affinity_entities[i], static_cast<int>(i % threads));

            auto start_time = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(TEST_DURATION);
            affinity_worker.Stop();
            auto end_time = std::chrono::high_resolution_clock::now();

            double affinity_elapsed = std::chrono::duration<double>(end_time - start_time).count();
            uint64_t total_affinity_ops = 0;
            for (const auto& task : affinity_entities)
                total_affinity_ops += task.run_count.load(std::memory_order_relaxed);
            double affinity_ops_per_sec = total_affinity_ops / affinity_elapsed;

            // -------------------------------------------------------------
            // 测试 2: SharedJobWorker
            // -------------------------------------------------------------
            std::vector<EntityTask> shared_entities(ENTITY_COUNT);
            for (auto& e : shared_entities)
                e.Randomize(rng);

            EntitySharedWorker shared_worker;
            shared_worker.Start(threads);

            for (size_t i = 0; i < ENTITY_COUNT; ++i)
                shared_worker.PushTask(&shared_entities[i]);

            start_time = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(TEST_DURATION);
            shared_worker.Stop();
            end_time = std::chrono::high_resolution_clock::now();

            double shared_elapsed = std::chrono::duration<double>(end_time - start_time).count();
            uint64_t total_shared_ops = 0;
            for (const auto& task : shared_entities)
                total_shared_ops += task.run_count.load(std::memory_order_relaxed);
            double shared_ops_per_sec = total_shared_ops / shared_elapsed;

            // 累加汇总
            sum_affinity_ops += total_affinity_ops;
            sum_shared_ops += total_shared_ops;

            // 百分比差异（以 SharedJobWorker 为基准：1 比 2 快/慢多少 %）
            double gain_percent = 0.0;
            if (shared_ops_per_sec > 0.0)
                gain_percent = (affinity_ops_per_sec - shared_ops_per_sec) / shared_ops_per_sec * 100.0;

            if (!any_result || gain_percent > max_gain_percent)
            {
                max_gain_percent = gain_percent;
                max_gain_threads = threads;
                any_result = true;
            }

            // 输出结果（带 Diff% 列）
            std::ostringstream diff_ss;
            diff_ss << std::fixed << std::setprecision(1)
                    << (gain_percent > 0.0 ? "+" : "")
                    << gain_percent << "%";

            std::cout << std::left
                      << std::setw(12) << threads
                      << std::setw(28) << static_cast<uint64_t>(affinity_ops_per_sec)
                      << std::setw(28) << static_cast<uint64_t>(shared_ops_per_sec)
                      << std::setw(14) << diff_ss.str()
                      << "\n";
        }

        std::cout << "------------------------------------------------------------------------\n";

        // 汇总统计
        if (any_result)
        {
            std::cout << "\n========== 汇总统计 ==========\n";
            std::cout << "AffinityWorker 总操作数  : " << sum_affinity_ops << "\n";
            std::cout << "SharedJobWorker 总操作数 : " << sum_shared_ops << "\n";

            if (sum_shared_ops > 0)
            {
                double overall_gain = (static_cast<double>(sum_affinity_ops) - static_cast<double>(sum_shared_ops))
                                      / static_cast<double>(sum_shared_ops) * 100.0;
                std::cout << std::fixed << std::setprecision(1);
                if (overall_gain >= 0.0)
                    std::cout << "AffinityWorker 整体比 SharedJobWorker 快 " << overall_gain << "%\n";
                else
                    std::cout << "AffinityWorker 整体比 SharedJobWorker 慢 " << std::fabs(overall_gain) << "%\n";
            }
            std::cout << "最大优势出现在 " << max_gain_threads << " 线程配置: " << max_gain_percent << "%\n";
        }

        std::cout << "\nBenchmark Complete.\n\n";
    }
}