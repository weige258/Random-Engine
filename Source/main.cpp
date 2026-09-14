#include "Application.hpp"
#include "Core/Objects/BaseObject/BaseObject.hpp"
#include "Core/Behaviors/BaseBehavior/BindBaseBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Core/Behaviors/BaseBehavior/IFixUpdateBehavior.hpp"
#include "Core/Config.hpp"
#include "Core/Jobs/Job/BaseJob.hpp"
#include "Core/Math/Math.hpp"
#include "Core/Memory/MasterPtr.hpp"
#include "Core/Memory/ObserverPtr.hpp"
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cmath>
#include <array>

using namespace RandomEngine::Core::Behaviors;
using namespace RandomEngine::Core::Jobs;
using namespace RandomEngine::Core::Objects;
using namespace RandomEngine::Core::Math;

struct EntityData
{
    Vec3d position;
    Vec3d velocity;
    Vec3d acceleration;
    Vec3d force;
    float mass;
    float radius;
    float restitution;
    float drag;
    Vec3d angular_velocity;
    Vec3d torque;
    float moment_of_inertia;
    Vec3d gravity;
    float max_speed;
};

struct alignas(64) PerfCounters
{
    alignas(64) std::atomic<uint64_t> movement{0};
    alignas(64) std::atomic<uint64_t> collision{0};
    alignas(64) std::atomic<uint64_t> bounce{0};
    alignas(64) std::atomic<uint64_t> drag{0};
    alignas(64) std::atomic<uint64_t> gravity{0};
    alignas(64) std::atomic<uint64_t> rotation{0};
    alignas(64) std::atomic<uint64_t> boundary{0};
    alignas(64) std::atomic<uint64_t> force_accum{0};
    alignas(64) std::atomic<uint64_t> spring{0};
    alignas(64) std::atomic<uint64_t> damping{0};
};

static PerfCounters g_counters;

struct MovementBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        data->velocity += data->acceleration * delta_time;
        float speed_sq = LengthSquared(data->velocity);
        if (speed_sq > data->max_speed * data->max_speed)
        {
            data->velocity = Normalize(data->velocity) * data->max_speed;
        }
        data->position += data->velocity * delta_time;
        data->acceleration = data->force / data->mass;
        data->force = Vec3d(0.0f, 0.0f, 0.0f);
        g_counters.movement.fetch_add(1, std::memory_order_relaxed);
    }
};

struct CollisionBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        Vec3f neighbor_offset(data->position[1] * 0.3f - data->position[0] * 0.1f,
                              data->position[2] * 0.2f - data->position[1] * 0.15f,
                              data->position[0] * 0.25f - data->position[2] * 0.12f);
        float dist_sq = LengthSquared(neighbor_offset);
        float min_dist = data->radius * 2.0f;
        if (dist_sq < min_dist * min_dist && dist_sq > 1e-8f)
        {
            float dist = std::sqrt(dist_sq);
            Vec3d normal = neighbor_offset / dist;
            float overlap = min_dist - dist;
            data->force += normal * (overlap * 500.0f);
            Vec3d rel_vel = data->velocity;
            float vel_along_normal = Dot(rel_vel, normal);
            if (vel_along_normal < 0.0f)
            {
                float j = -(1.0f + data->restitution) * vel_along_normal;
                j /= (1.0f / data->mass + 1.0f / data->mass);
                data->velocity += normal * (j / data->mass);
            }
        }
        g_counters.collision.fetch_add(1, std::memory_order_relaxed);
    }
};

struct BounceBehavior : BindBaseBehavior, IFixUpdateBehavior
{
    void FixUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        constexpr float floor_y = -50.0f;
        constexpr float ceil_y = 50.0f;
        if (data->position.Y() - data->radius < floor_y)
        {
            data->position.SetY(floor_y + data->radius);
            data->velocity.SetY(-data->velocity.Y() * data->restitution);
            data->velocity.SetX(data->velocity.X() * 0.98f);
            data->velocity.SetZ(data->velocity.Z() * 0.98f);
        }
        if (data->position.Y() + data->radius > ceil_y)
        {
            data->position.SetY(ceil_y - data->radius);
            data->velocity.SetY(-data->velocity.Y() * data->restitution);
        }
        g_counters.bounce.fetch_add(1, std::memory_order_relaxed);
    }
};

struct DragBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        float speed = std::sqrt(LengthSquared(data->velocity));
        if (speed > 1e-6f)
        {
            Vec3d drag_force = Normalize(data->velocity) * (-data->drag * speed * speed);
            data->force += drag_force;
        }
        g_counters.drag.fetch_add(1, std::memory_order_relaxed);
    }
};

struct GravityBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        data->force += data->gravity * data->mass;
        g_counters.gravity.fetch_add(1, std::memory_order_relaxed);
    }
};

struct RotationBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        float angular_speed = std::sqrt(LengthSquared(data->angular_velocity));
        if (angular_speed > 1e-8f)
        {
            Vec3d angular_drag = Normalize(data->angular_velocity) * (-0.5f * angular_speed);
            data->torque += angular_drag;
        }
        data->angular_velocity += (data->torque / data->moment_of_inertia) * delta_time;
        data->torque = Vec3d(0.0f, 0.0f, 0.0f);
        g_counters.rotation.fetch_add(1, std::memory_order_relaxed);
    }
};

struct BoundaryBehavior : BindBaseBehavior, IFixUpdateBehavior
{
    void FixUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        constexpr float bound = 100.0f;
        constexpr float k_wall = 1000.0f;
        for (int i = 0; i < 3; ++i)
        {
            if (data->position[i] > bound)
            {
                data->force[i] -= k_wall * (data->position[i] - bound);
                data->velocity[i] *= -0.5f;
            }
            else if (data->position[i] < -bound)
            {
                data->force[i] -= k_wall * (data->position[i] + bound);
                data->velocity[i] *= -0.5f;
            }
        }
        g_counters.boundary.fetch_add(1, std::memory_order_relaxed);
    }
};

struct ForceAccumBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        Vec3d spring_anchor(0.0f, 0.0f, 0.0f);
        Vec3d displacement = data->position - spring_anchor;
        float dist = std::sqrt(LengthSquared(displacement));
        if (dist > 1e-6f)
        {
            Vec3d spring_force = Normalize(displacement) * (-2.0f * dist);
            data->force += spring_force;
        }
        g_counters.force_accum.fetch_add(1, std::memory_order_relaxed);
    }
};

struct SpringBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        Vec3d neighbor_pos(data->position[1] * 0.5f + 10.0f,
                           data->position[2] * 0.3f - 5.0f,
                           data->position[0] * 0.4f + 8.0f);
        Vec3d diff = neighbor_pos - data->position;
        float rest_length = 5.0f;
        float current_length = std::sqrt(LengthSquared(diff));
        if (current_length > 1e-6f)
        {
            float stretch = current_length - rest_length;
            Vec3d spring_f = Normalize(diff) * (3.0f * stretch);
            data->force += spring_f;
        }
        g_counters.spring.fetch_add(1, std::memory_order_relaxed);
    }
};

struct DampingBehavior : BindBaseBehavior, ILogicUpdateBehavior
{
    void LogicUpdate(RandomEngine::Core::Config::TimeType delta_time, RandomEngine::Systems::System &system) 
    {
        auto data = system.resource_system.object_system.GetLocked<EntityData>(bind_id);
        data->force -= data->velocity * 0.8f;
        data->torque -= data->angular_velocity * 0.3f;
        g_counters.damping.fetch_add(1, std::memory_order_relaxed);
    }
};

struct TestEntity : BaseObject, EntityData
{
    BIND_BEHAVIORS(
        MovementBehavior,
        CollisionBehavior,
        BounceBehavior,
        DragBehavior,
        GravityBehavior,
        RotationBehavior,
        BoundaryBehavior,
        ForceAccumBehavior,
        SpringBehavior,
        DampingBehavior)
};

int main()
{
    RandomEngine::Engine::Application app;
    app.Init();

    constexpr int NUM_ENTITIES = 100000;
    std::printf("Creating %d entities with 10 behaviors each...\n", NUM_ENTITIES);

    for (int i = 0; i < NUM_ENTITIES; ++i)
    {
        TestEntity entity;
        float fi = static_cast<float>(i);
        float x = std::sin(fi * 0.1f) * 50.0f;
        float y = std::cos(fi * 0.13f) * 30.0f + 20.0f;
        float z = std::sin(fi * 0.07f + 1.0f) * 40.0f;
        entity.position = Vec3f(x, y, z);
        entity.velocity = Vec3f(std::cos(fi * 0.3f) * 5.0f,
                                std::sin(fi * 0.2f) * 3.0f,
                                std::cos(fi * 0.25f + 2.0f) * 4.0f);
        entity.acceleration = Vec3f(0.0f, 0.0f, 0.0f);
        entity.force = Vec3f(0.0f, 0.0f, 0.0f);
        entity.mass = 1.0f + std::fmod(fi * 0.1f, 5.0f);
        entity.radius = 0.5f + std::fmod(fi * 0.01f, 1.0f);
        entity.restitution = 0.6f + std::fmod(fi * 0.003f, 0.3f);
        entity.drag = 0.1f + std::fmod(fi * 0.002f, 0.2f);
        entity.angular_velocity = Vec3f(std::sin(fi * 0.15f) * 2.0f,
                                        std::cos(fi * 0.12f) * 1.5f,
                                        std::sin(fi * 0.09f + 1.0f) * 1.8f);
        entity.torque = Vec3f(0.0f, 0.0f, 0.0f);
        entity.moment_of_inertia = entity.mass * 0.4f;
        entity.gravity = Vec3f(0.0f, -9.81f, 0.0f);
        entity.max_speed = 50.0f;
        app.system.Get()->resource_system.Add(std::move(entity));
    }

    std::printf("Entities created. Starting performance test...\n\n");

    auto last_time = std::chrono::steady_clock::now();

    while (true)
    {
        app.system.Get()->Run(*app.system.Get());

        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - last_time).count();

        if (elapsed >= 1.0f)
        {
            uint64_t m = g_counters.movement.load(std::memory_order_relaxed);
            uint64_t c = g_counters.collision.load(std::memory_order_relaxed);
            uint64_t b = g_counters.bounce.load(std::memory_order_relaxed);
            uint64_t d = g_counters.drag.load(std::memory_order_relaxed);
            uint64_t g = g_counters.gravity.load(std::memory_order_relaxed);
            uint64_t r = g_counters.rotation.load(std::memory_order_relaxed);
            uint64_t bd = g_counters.boundary.load(std::memory_order_relaxed);
            uint64_t fa = g_counters.force_accum.load(std::memory_order_relaxed);
            uint64_t sp = g_counters.spring.load(std::memory_order_relaxed);
            uint64_t dm = g_counters.damping.load(std::memory_order_relaxed);

            float inv_t = 1.0f / elapsed;
            uint64_t total = m + c + b + d + g + r + bd + fa + sp + dm;

            std::printf("--- %.1fs ---\n", elapsed);
            std::printf("  Movement:    %12llu  (%.0f/s)\n", (unsigned long long)m, m * inv_t);
            std::printf("  Collision:   %12llu  (%.0f/s)\n", (unsigned long long)c, c * inv_t);
            std::printf("  Bounce:      %12llu  (%.0f/s)\n", (unsigned long long)b, b * inv_t);
            std::printf("  Drag:        %12llu  (%.0f/s)\n", (unsigned long long)d, d * inv_t);
            std::printf("  Gravity:     %12llu  (%.0f/s)\n", (unsigned long long)g, g * inv_t);
            std::printf("  Rotation:    %12llu  (%.0f/s)\n", (unsigned long long)r, r * inv_t);
            std::printf("  Boundary:    %12llu  (%.0f/s)\n", (unsigned long long)bd, bd * inv_t);
            std::printf("  ForceAccum:  %12llu  (%.0f/s)\n", (unsigned long long)fa, fa * inv_t);
            std::printf("  Spring:      %12llu  (%.0f/s)\n", (unsigned long long)sp, sp * inv_t);
            std::printf("  Damping:     %12llu  (%.0f/s)\n", (unsigned long long)dm, dm * inv_t);
            std::printf("  TOTAL:       %12llu  (%.0f/s)\n\n", (unsigned long long)total, total * inv_t);

            last_time = now;
            g_counters.movement.store(0, std::memory_order_relaxed);
            g_counters.collision.store(0, std::memory_order_relaxed);
            g_counters.bounce.store(0, std::memory_order_relaxed);
            g_counters.drag.store(0, std::memory_order_relaxed);
            g_counters.gravity.store(0, std::memory_order_relaxed);
            g_counters.rotation.store(0, std::memory_order_relaxed);
            g_counters.boundary.store(0, std::memory_order_relaxed);
            g_counters.force_accum.store(0, std::memory_order_relaxed);
            g_counters.spring.store(0, std::memory_order_relaxed);
            g_counters.damping.store(0, std::memory_order_relaxed);
        }
    }

    app.Destroy();
    return 0;
}