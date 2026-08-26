#pragma once

#include "Behaviors/BaseBehavior/BindBaseBehavior.hpp"
#include "Behaviors/BaseBehavior/ILogicUpdateBehavior.hpp"
#include "Behaviors/BaseBehavior/IFixUpdateBehavior.hpp"
#include "Behaviors/BaseBehavior/ISystemUpdateBehavior.hpp"
#include "Behaviors/BaseBehavior/StaticBehaviors.hpp"
#include "Behaviors/BaseBehavior/DynamicBehaviors.hpp"
#include "Objects/BaseObject/BaseObject.hpp"
#include "Systems/System.hpp"
#include "Core/Math/Vec.hpp"
#include "Core/Math/MathFunctions.hpp"
#include "BindBehaviors.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>
#include <atomic>
#include <array>
#include <chrono>
#include <iomanip>

namespace RandEngine::Core::Objects {

namespace Perf {

struct BPSSpinLock {
    std::atomic<bool> flag{false};

    void Lock() noexcept {
        while (flag.exchange(true, std::memory_order_acquire)) {
            #if defined(_MSC_VER)
            _mm_pause();
            #else
            __builtin_ia32_pause();
            #endif
        }
    }

    void Unlock() noexcept {
        flag.store(false, std::memory_order_release);
    }
};

struct BPSScopedLock {
    BPSSpinLock& lk;
    BPSScopedLock(BPSSpinLock& l) noexcept : lk(l) { lk.Lock(); }
    ~BPSScopedLock() noexcept { lk.Unlock(); }
};

struct alignas(64) BehaviorBPS {
    std::atomic<uint64_t> call_count{0};
    std::chrono::high_resolution_clock::time_point last_report;
    float report_interval{1.0f};
    bool initialized{false};
    BPSSpinLock output_lock;

    static BehaviorBPS& Instance() noexcept {
        static BehaviorBPS inst;
        return inst;
    }

    void Record() noexcept {
        call_count.fetch_add(1, std::memory_order_relaxed);
    }

    void Tick() noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        if (!initialized) {
            initialized = true;
            last_report = now;
            return;
        }
        float elapsed = std::chrono::duration<float>(now - last_report).count();
        if (elapsed >= report_interval) {
            uint64_t calls = call_count.exchange(0, std::memory_order_relaxed);
            float bps = (elapsed > 0.0f) ? static_cast<float>(calls) / elapsed : 0.0f;
            last_report = now;
            BPSScopedLock guard(output_lock);
            std::cout << "Behaviors/s: " << std::fixed << std::setprecision(0) << bps << "\n" << std::flush;
        }
    }
};

} // namespace Perf

using Vec2f = Math::Vec<float, 2>;
using Vec3f = Math::Vec<float, 3>;

namespace Collision {

struct AABB {
    Vec2f center;
    Vec2f half_extents;

    Vec2f Min() const { return center - half_extents; }
    Vec2f Max() const { return center + half_extents; }

    bool Contains(const Vec2f& point) const {
        auto mn = Min();
        auto mx = Max();
        return point[0] >= mn[0] && point[0] <= mx[0] &&
               point[1] >= mn[1] && point[1] <= mx[1];
    }

    static bool Intersects(const AABB& a, const AABB& b) {
        auto a_mn = a.Min(), a_mx = a.Max();
        auto b_mn = b.Min(), b_mx = b.Max();
        return a_mn[0] <= b_mx[0] && a_mx[0] >= b_mn[0] &&
               a_mn[1] <= b_mx[1] && a_mx[1] >= b_mn[1];
    }
};

struct Circle {
    Vec2f center;
    float radius;

    bool Contains(const Vec2f& point) const {
        auto diff = point - center;
        return Math::LengthSquared(diff) <= radius * radius;
    }

    static bool Intersects(const Circle& a, const Circle& b) {
        float dist_sq = Math::LengthSquared(a.center - b.center);
        float radius_sum = a.radius + b.radius;
        return dist_sq <= radius_sum * radius_sum;
    }
};

struct Sphere {
    Vec3f center;
    float radius;

    bool Contains(const Vec3f& point) const {
        auto diff = point - center;
        return Math::LengthSquared(diff) <= radius * radius;
    }

    static bool Intersects(const Sphere& a, const Sphere& b) {
        float dist_sq = Math::LengthSquared(a.center - b.center);
        float radius_sum = a.radius + b.radius;
        return dist_sq <= radius_sum * radius_sum;
    }
};

struct Ray2D {
    Vec2f origin;
    Vec2f direction;

    struct Hit {
        bool occurred = false;
        float distance = 0.0f;
        Vec2f point;
        Vec2f normal;
    };

    Hit IntersectAABB(const AABB& box) const {
        Hit hit;
        auto mn = box.Min();
        auto mx = box.Max();

        float tmin = -std::numeric_limits<float>::infinity();
        float tmax = std::numeric_limits<float>::infinity();

        for (int i = 0; i < 2; ++i) {
            if (std::abs(direction[i]) < 1e-8f) {
                if (origin[i] < mn[i] || origin[i] > mx[i]) {
                    hit.occurred = false;
                    return hit;
                }
            } else {
                float t1 = (mn[i] - origin[i]) / direction[i];
                float t2 = (mx[i] - origin[i]) / direction[i];
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax) {
                    hit.occurred = false;
                    return hit;
                }
            }
        }

        if (tmin < 0.0f) tmin = tmax;
        if (tmin < 0.0f) {
            hit.occurred = false;
            return hit;
        }

        hit.occurred = true;
        hit.distance = tmin;
        hit.point = origin + direction * tmin;

        auto diff = hit.point - box.center;
        float sx = std::abs(diff[0]) / box.half_extents[0];
        float sy = std::abs(diff[1]) / box.half_extents[1];
        if (sx > sy) {
            hit.normal = Vec2f(diff[0] > 0 ? 1.0f : -1.0f, 0.0f);
        } else {
            hit.normal = Vec2f(0.0f, diff[1] > 0 ? 1.0f : -1.0f);
        }

        return hit;
    }

    Hit IntersectCircle(const Circle& circle) const {
        Hit hit;
        auto oc = origin - circle.center;
        float a = Math::LengthSquared(direction);
        float b = 2.0f * Math::Dot(oc, direction);
        float c = Math::LengthSquared(oc) - circle.radius * circle.radius;
        float discriminant = b * b - 4.0f * a * c;

        if (discriminant < 0.0f) {
            hit.occurred = false;
            return hit;
        }

        float sqrt_disc = std::sqrt(discriminant);
        float t0 = (-b - sqrt_disc) / (2.0f * a);
        float t1 = (-b + sqrt_disc) / (2.0f * a);
        float t = (t0 >= 0.0f) ? t0 : t1;

        if (t < 0.0f) {
            hit.occurred = false;
            return hit;
        }

        hit.occurred = true;
        hit.distance = t;
        hit.point = origin + direction * t;
        auto diff = hit.point - circle.center;
        hit.normal = Math::Normalize(diff);

        return hit;
    }
};

struct CollisionInfo {
    bool collided = false;
    Vec2f penetration_normal;
    float penetration_depth = 0.0f;
    Vec2f contact_point;
};

inline CollisionInfo ResolveAABB(const AABB& a, const AABB& b) {
    CollisionInfo info;
    auto a_mn = a.Min(), a_mx = a.Max();
    auto b_mn = b.Min(), b_mx = b.Max();

    float overlap_x = std::min(a_mx[0], b_mx[0]) - std::max(a_mn[0], b_mn[0]);
    float overlap_y = std::min(a_mx[1], b_mx[1]) - std::max(a_mn[1], b_mn[1]);

    if (overlap_x <= 0.0f || overlap_y <= 0.0f) {
        info.collided = false;
        return info;
    }

    info.collided = true;
    info.contact_point = (a.center + b.center) * 0.5f;

    if (overlap_x < overlap_y) {
        info.penetration_depth = overlap_x;
        info.penetration_normal = Vec2f(a.center[0] < b.center[0] ? -1.0f : 1.0f, 0.0f);
    } else {
        info.penetration_depth = overlap_y;
        info.penetration_normal = Vec2f(0.0f, a.center[1] < b.center[1] ? -1.0f : 1.0f);
    }

    return info;
}

inline CollisionInfo ResolveCircles(const Circle& a, const Circle& b) {
    CollisionInfo info;
    auto diff = b.center - a.center;
    float dist_sq = Math::LengthSquared(diff);
    float radius_sum = a.radius + b.radius;

    if (dist_sq >= radius_sum * radius_sum) {
        info.collided = false;
        return info;
    }

    float dist = std::sqrt(dist_sq);
    info.collided = true;
    info.penetration_depth = radius_sum - dist;

    if (dist < 1e-8f) {
        info.penetration_normal = Vec2f(1.0f, 0.0f);
    } else {
        info.penetration_normal = diff * (-1.0f / dist);
    }

    float t = a.radius / radius_sum;
    info.contact_point = a.center + diff * t;

    return info;
}

} // namespace Collision

struct TestEntity;

struct TransformData {
    Vec2f position{0.0f, 0.0f};
    Vec2f velocity{0.0f, 0.0f};
    Vec2f acceleration{0.0f, 0.0f};
    float rotation = 0.0f;
    float angular_velocity = 0.0f;
    float scale = 1.0f;

    void Integrate(float dt) {
        velocity = velocity + acceleration * dt;
        position = position + velocity * dt;
        rotation += angular_velocity * dt;
    }
};

struct PhysicsData {
    float mass = 1.0f;
    float restitution = 0.5f;
    float friction = 0.1f;
    float drag = 0.01f;
    bool is_kinematic = false;

    Vec2f ComputeImpulse(const PhysicsData& other, const Vec2f& normal, float relative_speed) const {
        float e = std::min(restitution, other.restitution);
        float j = -(1.0f + e) * relative_speed;
        j /= (1.0f / mass + 1.0f / other.mass);
        return normal * j;
    }

    void ApplyDrag(Vec2f& velocity, float dt) const {
        if (!is_kinematic) {
            velocity = velocity * (1.0f - drag * dt);
        }
    }
};

struct HealthData {
    float current = 100.0f;
    float max_value = 100.0f;
    bool is_alive = true;

    void TakeDamage(float amount) {
        current -= amount;
        if (current <= 0.0f) {
            current = 0.0f;
            is_alive = false;
        }
    }

    void Heal(float amount) {
        current = std::min(current + amount, max_value);
    }

    float Ratio() const { return current / max_value; }
};

struct TagData {
    static constexpr int Player = 0;
    static constexpr int Enemy = 1;
    static constexpr int Projectile = 2;
    static constexpr int Obstacle = 3;
    static constexpr int Collectible = 4;
    static constexpr int Trigger = 5;
    static constexpr int NPC = 6;

    int tag = -1;
    int layer = 0;
    uint32_t collision_mask = 0xFFFFFFFF;
};

struct MovementBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    PhysicsData* physics = nullptr;
    Vec2f target_position{0.0f, 0.0f};
    float move_speed = 100.0f;
    float arrival_threshold = 2.0f;
    bool has_target = false;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !physics || physics->is_kinematic) return;

        if (has_target) {
            auto diff = target_position - transform->position;
            float dist = Math::Length(diff);
            if (dist > arrival_threshold) {
                auto dir = diff * (1.0f / dist);
                transform->velocity = dir * move_speed;
            } else {
                transform->velocity = Vec2f(0.0f, 0.0f);
                has_target = false;
            }
        }

        physics->ApplyDrag(transform->velocity, delta_time);
        transform->Integrate(delta_time);
    }
};

struct GravityBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    PhysicsData* physics = nullptr;
    float gravity_strength = 980.0f;
    Vec2f gravity_direction{0.0f, -1.0f};
    bool enabled = true;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !physics || physics->is_kinematic || !enabled) return;
        transform->acceleration = transform->acceleration + gravity_direction * gravity_strength;
    }
};

struct CollisionBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    PhysicsData* physics = nullptr;
    TagData* tag = nullptr;
    Collision::AABB bounds;
    Collision::Circle circle_bounds;
    bool use_aabb = true;
    bool use_circle = false;

    std::vector<TransformData*> other_transforms;
    std::vector<PhysicsData*> other_physics;
    std::vector<TagData*> other_tags;
    std::vector<Collision::AABB> other_bounds;
    std::vector<Collision::Circle> other_circles;
    std::vector<bool> other_use_aabb;

    std::function<void(const Collision::CollisionInfo&, int)> on_collision;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !physics) return;

        if (use_aabb) {
            bounds.center = transform->position;
        }
        if (use_circle) {
            circle_bounds.center = transform->position;
        }

        for (size_t i = 0; i < other_transforms.size(); ++i) {
            if (!other_tags[i] || !(tag->collision_mask & (1u << other_tags[i]->tag))) continue;

            Collision::CollisionInfo info;

            if (use_aabb && other_use_aabb[i]) {
                auto other_aabb = other_bounds[i];
                other_aabb.center = other_transforms[i]->position;
                info = Collision::ResolveAABB(bounds, other_aabb);
            } else if (use_circle && !other_use_aabb[i]) {
                auto other_circle = other_circles[i];
                other_circle.center = other_transforms[i]->position;
                info = Collision::ResolveCircles(circle_bounds, other_circle);
            }

            if (info.collided) {
                if (!physics->is_kinematic && other_physics[i]) {
                    auto separation = info.penetration_normal * info.penetration_depth;
                    transform->position = transform->position + separation;

                    float relative_speed = Math::Dot(
                        transform->velocity - other_transforms[i]->velocity,
                        info.penetration_normal
                    );

                    if (relative_speed < 0.0f) {
                        auto impulse = physics->ComputeImpulse(*other_physics[i], info.penetration_normal, relative_speed);
                        transform->velocity = transform->velocity + impulse * (1.0f / physics->mass);
                    }
                }

                if (on_collision) {
                    on_collision(info, static_cast<int>(i));
                }
            }
        }
    }
};

struct PatrolBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    std::vector<Vec2f> waypoints;
    size_t current_waypoint = 0;
    float patrol_speed = 60.0f;
    float arrival_radius = 5.0f;
    bool loop = true;
    bool paused = false;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || waypoints.empty() || paused) return;

        auto& target = waypoints[current_waypoint];
        auto diff = target - transform->position;
        float dist = Math::Length(diff);

        if (dist <= arrival_radius) {
            if (current_waypoint + 1 < waypoints.size()) {
                current_waypoint++;
            } else if (loop) {
                current_waypoint = 0;
            } else {
                transform->velocity = Vec2f(0.0f, 0.0f);
                return;
            }
        }

        if (dist > 1e-8f) {
            auto dir = diff * (1.0f / dist);
            transform->velocity = dir * patrol_speed;
        }
    }
};

struct ChaseBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    TransformData* target = nullptr;
    float chase_speed = 120.0f;
    float detection_radius = 300.0f;
    float attack_radius = 30.0f;
    bool is_chasing = false;

    std::function<void()> on_enter_chase;
    std::function<void()> on_exit_chase;
    std::function<void()> on_attack;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !target) return;

        auto diff = target->position - transform->position;
        float dist = Math::Length(diff);

        if (dist <= attack_radius) {
            transform->velocity = Vec2f(0.0f, 0.0f);
            if (on_attack) on_attack();
            return;
        }

        if (dist <= detection_radius) {
            if (!is_chasing) {
                is_chasing = true;
                if (on_enter_chase) on_enter_chase();
            }
            auto dir = diff * (1.0f / dist);
            transform->velocity = dir * chase_speed;
        } else {
            if (is_chasing) {
                is_chasing = false;
                transform->velocity = Vec2f(0.0f, 0.0f);
                if (on_exit_chase) on_exit_chase();
            }
        }
    }
};

struct ProjectileBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    HealthData* health = nullptr;
    Vec2f direction{1.0f, 0.0f};
    float speed = 500.0f;
    float lifetime = 3.0f;
    float elapsed = 0.0f;
    float damage = 25.0f;
    bool active = true;

    std::function<void()> on_expire;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !active) return;

        transform->velocity = direction * speed;
        transform->Integrate(delta_time);

        elapsed += delta_time;
        if (elapsed >= lifetime) {
            active = false;
            if (health) health->is_alive = false;
            if (on_expire) on_expire();
        }
    }

    void Reset(const Vec2f& start, const Vec2f& dir) {
        if (!transform) return;
        transform->position = start;
        direction = dir;
        elapsed = 0.0f;
        active = true;
        if (health) {
            health->is_alive = true;
            health->current = health->max_value;
        }
    }
};

struct HealthBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    HealthData* health = nullptr;
    float regen_rate = 0.0f;
    float invincibility_time = 0.0f;
    float invincibility_timer = 0.0f;

    std::function<void()> on_death;
    std::function<void(float)> on_damage_taken;
    std::function<void(float)> on_healed;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!health || !health->is_alive) return;

        if (invincibility_timer > 0.0f) {
            invincibility_timer -= delta_time;
        }

        if (regen_rate > 0.0f && health->current < health->max_value) {
            float old_hp = health->current;
            health->Heal(regen_rate * delta_time);
            if (on_healed) on_healed(health->current - old_hp);
        }
    }

    void ApplyDamage(float amount) {
        if (!health || !health->is_alive || invincibility_timer > 0.0f) return;
        health->TakeDamage(amount);
        invincibility_timer = invincibility_time;
        if (on_damage_taken) on_damage_taken(amount);
        if (!health->is_alive && on_death) on_death();
    }
};

struct CollectibleBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    float bob_amplitude = 5.0f;
    float bob_frequency = 2.0f;
    float rotation_speed = 90.0f;
    float elapsed = 0.0f;
    bool collected = false;
    Vec2f base_position{0.0f, 0.0f};

    std::function<void()> on_collected;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || collected) return;

        elapsed += delta_time;
        float bob_offset = std::sin(elapsed * bob_frequency * 2.0f * 3.14159265f) * bob_amplitude;
        transform->position = Vec2f(base_position[0], base_position[1] + bob_offset);
        transform->rotation += rotation_speed * delta_time;
    }

    void Collect() {
        if (collected) return;
        collected = true;
        if (on_collected) on_collected();
    }
};

struct TriggerZoneBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    Collision::AABB zone;
    std::vector<TransformData*> monitored_entities;
    std::vector<bool> inside_flags;
    bool one_shot = false;
    bool triggered = false;

    std::function<void(int)> on_enter;
    std::function<void(int)> on_exit;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        for (size_t i = 0; i < monitored_entities.size(); ++i) {
            bool was_inside = (i < inside_flags.size()) ? inside_flags[i] : false;
            bool is_inside = zone.Contains(monitored_entities[i]->position);

            if (is_inside && !was_inside) {
                if (on_enter) on_enter(static_cast<int>(i));
                if (one_shot) triggered = true;
            } else if (!is_inside && was_inside) {
                if (on_exit) on_exit(static_cast<int>(i));
            }

            if (i < inside_flags.size()) {
                inside_flags[i] = is_inside;
            } else {
                inside_flags.push_back(is_inside);
            }
        }
    }
};

struct OscillationBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    Vec2f axis{0.0f, 1.0f};
    float amplitude = 50.0f;
    float frequency = 1.0f;
    float phase = 0.0f;
    float elapsed = 0.0f;
    Vec2f origin{0.0f, 0.0f};

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform) return;
        elapsed += delta_time;
        float offset = std::sin(elapsed * frequency * 2.0f * 3.14159265f + phase) * amplitude;
        transform->position = origin + axis * offset;
    }
};

struct AreaDamageBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    float radius = 100.0f;
    float damage_per_second = 10.0f;
    bool active = true;

    std::vector<TransformData*> targets;
    std::vector<HealthData*> target_healths;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !active) return;

        for (size_t i = 0; i < targets.size(); ++i) {
            if (!target_healths[i] || !target_healths[i]->is_alive) continue;
            float dist = Math::Length(targets[i]->position - transform->position);
            if (dist <= radius) {
                float falloff = 1.0f - (dist / radius);
                target_healths[i]->TakeDamage(damage_per_second * falloff * delta_time);
            }
        }
    }
};

struct TestPlayerEntity;

struct PlayerMovementBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    PhysicsData* physics = nullptr;
    float move_speed = 200.0f;
    float jump_impulse = 400.0f;
    bool grounded = false;

    bool input_left = false;
    bool input_right = false;
    bool input_jump = false;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !physics || physics->is_kinematic) return;

        Vec2f move_dir{0.0f, 0.0f};
        if (input_left) move_dir[0] -= 1.0f;
        if (input_right) move_dir[0] += 1.0f;

        transform->velocity[0] = move_dir[0] * move_speed;

        if (input_jump && grounded) {
            transform->velocity[1] = jump_impulse;
            grounded = false;
        }

        physics->ApplyDrag(transform->velocity, delta_time);
        transform->Integrate(delta_time);
    }
};

struct TestPlayerEntity : virtual BaseObject,
                         virtual BindStaticBehaviors<PlayerMovementBehavior, GravityBehavior, HealthBehavior>,
                         virtual BindDynamicBehaviors<PlayerMovementBehavior, GravityBehavior, HealthBehavior> {
    TransformData transform;
    PhysicsData physics;
    HealthData health;
    TagData tag;

    TestPlayerEntity() {
        tag.tag = TagData::Player;
        tag.layer = 0;
        physics.mass = 1.0f;
        physics.restitution = 0.3f;
        physics.drag = 0.02f;
        health.max_value = 100.0f;
        health.current = 100.0f;
    }
};

struct TestEnemyEntity;

struct EnemyAIBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    TransformData* player_transform = nullptr;
    HealthData* health = nullptr;
    float chase_speed = 80.0f;
    float detection_range = 250.0f;
    float attack_range = 25.0f;
    float attack_damage = 15.0f;
    float attack_cooldown = 1.0f;
    float attack_timer = 0.0f;
    HealthData* player_health = nullptr;

    enum class State { Idle, Chase, Attack, Flee };
    State state = State::Idle;

    std::function<void(State)> on_state_changed;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !health || !health->is_alive) return;

        attack_timer -= delta_time;

        if (health->Ratio() < 0.2f) {
            SetState(State::Flee);
        }

        switch (state) {
            case State::Idle: UpdateIdle(delta_time); break;
            case State::Chase: UpdateChase(delta_time); break;
            case State::Attack: UpdateAttack(delta_time); break;
            case State::Flee: UpdateFlee(delta_time); break;
        }
    }

    void SetState(State new_state) {
        if (state != new_state) {
            state = new_state;
            if (on_state_changed) on_state_changed(new_state);
        }
    }

    void UpdateIdle(float dt) {
        if (!player_transform) return;
        float dist = Math::Length(player_transform->position - transform->position);
        if (dist <= detection_range) {
            SetState(State::Chase);
        }
        transform->velocity = Vec2f(0.0f, 0.0f);
    }

    void UpdateChase(float dt) {
        if (!player_transform) return;
        auto diff = player_transform->position - transform->position;
        float dist = Math::Length(diff);

        if (dist > detection_range * 1.5f) {
            SetState(State::Idle);
            return;
        }
        if (dist <= attack_range) {
            SetState(State::Attack);
            return;
        }

        auto dir = diff * (1.0f / dist);
        transform->velocity = dir * chase_speed;
    }

    void UpdateAttack(float dt) {
        if (!player_transform) return;
        float dist = Math::Length(player_transform->position - transform->position);

        if (dist > attack_range * 1.5f) {
            SetState(State::Chase);
            return;
        }

        transform->velocity = Vec2f(0.0f, 0.0f);

        if (attack_timer <= 0.0f && player_health && player_health->is_alive) {
            player_health->TakeDamage(attack_damage);
            attack_timer = attack_cooldown;
        }
    }

    void UpdateFlee(float dt) {
        if (!player_transform) return;
        auto diff = transform->position - player_transform->position;
        float dist = Math::Length(diff);
        if (dist > 1e-8f) {
            auto dir = diff * (1.0f / dist);
            transform->velocity = dir * chase_speed * 1.2f;
        }
    }
};

struct TestEnemyEntity : virtual BaseObject,
                        virtual BindStaticBehaviors<EnemyAIBehavior, HealthBehavior>,
                        virtual BindDynamicBehaviors<EnemyAIBehavior, HealthBehavior> {
    TransformData transform;
    PhysicsData physics;
    HealthData health;
    TagData tag;

    TestEnemyEntity() {
        tag.tag = TagData::Enemy;
        tag.layer = 1;
        physics.mass = 2.0f;
        physics.restitution = 0.2f;
        physics.drag = 0.05f;
        health.max_value = 60.0f;
        health.current = 60.0f;
    }
};

struct TestProjectileEntity;

struct TestProjectileEntity : virtual BaseObject,
                             virtual BindStaticBehaviors<ProjectileBehavior>,
                             virtual BindDynamicBehaviors<ProjectileBehavior> {
    TransformData transform;
    HealthData health;
    TagData tag;

    TestProjectileEntity() {
        tag.tag = TagData::Projectile;
        tag.layer = 2;
        health.max_value = 1.0f;
        health.current = 1.0f;
    }
};

struct TestObstacleEntity;

struct TestObstacleEntity : virtual BaseObject {
    TransformData transform;
    PhysicsData physics;
    TagData tag;
    Collision::AABB bounds;

    TestObstacleEntity() {
        tag.tag = TagData::Obstacle;
        tag.layer = 3;
        physics.is_kinematic = true;
        physics.mass = 0.0f;
        physics.restitution = 1.0f;
    }
};

struct TestCollectibleEntity;

struct TestCollectibleEntity : virtual BaseObject,
                              virtual BindStaticBehaviors<CollectibleBehavior>,
                              virtual BindDynamicBehaviors<CollectibleBehavior> {
    TransformData transform;
    TagData tag;
    int value = 10;

    TestCollectibleEntity() {
        tag.tag = TagData::Collectible;
        tag.layer = 4;
    }
};

struct TestNPCEntity;

struct NPCDialogBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    TransformData* player_transform = nullptr;
    float interaction_radius = 50.0f;
    bool player_in_range = false;
    bool dialog_active = false;
    int current_line = 0;
    std::vector<std::string> dialog_lines;

    std::function<void(const std::string&)> on_show_dialog;
    std::function<void()> on_end_dialog;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        if (!transform || !player_transform) return;

        float dist = Math::Length(player_transform->position - transform->position);
        player_in_range = dist <= interaction_radius;
    }

    void StartDialog() {
        if (!player_in_range || dialog_lines.empty()) return;
        dialog_active = true;
        current_line = 0;
        if (on_show_dialog && current_line < static_cast<int>(dialog_lines.size())) {
            on_show_dialog(dialog_lines[current_line]);
        }
    }

    void AdvanceDialog() {
        if (!dialog_active) return;
        current_line++;
        if (current_line >= static_cast<int>(dialog_lines.size())) {
            dialog_active = false;
            current_line = 0;
            if (on_end_dialog) on_end_dialog();
        } else {
            if (on_show_dialog) on_show_dialog(dialog_lines[current_line]);
        }
    }
};

struct TestNPCEntity : virtual BaseObject,
                      virtual BindStaticBehaviors<PatrolBehavior, NPCDialogBehavior>,
                      virtual BindDynamicBehaviors<PatrolBehavior, NPCDialogBehavior> {
    TransformData transform;
    TagData tag;

    TestNPCEntity() {
        tag.tag = TagData::NPC;
        tag.layer = 5;
    }
};

struct TestTriggerEntity;

struct TestTriggerEntity : virtual BaseObject,
                          virtual BindStaticBehaviors<TriggerZoneBehavior>,
                          virtual BindDynamicBehaviors<TriggerZoneBehavior> {
    TagData tag;

    TestTriggerEntity() {
        tag.tag = TagData::Trigger;
        tag.layer = 6;
    }
};

struct TestScene;

struct TestScene {
    TestPlayerEntity player;
    std::vector<TestEnemyEntity> enemies;
    std::vector<TestProjectileEntity> projectiles;
    std::vector<TestObstacleEntity> obstacles;
    std::vector<TestCollectibleEntity> collectibles;
    std::vector<TestNPCEntity> npcs;
    std::vector<TestTriggerEntity> triggers;

    std::vector<Collision::CollisionInfo> frame_collisions;

    void SetupDefaultScene() {
        player.transform.position = Vec2f(0.0f, 0.0f);

        enemies.emplace_back();
        enemies.back().transform.position = Vec2f(200.0f, 0.0f);

        enemies.emplace_back();
        enemies.back().transform.position = Vec2f(-150.0f, 100.0f);

        obstacles.emplace_back();
        auto& wall = obstacles.back();
        wall.transform.position = Vec2f(0.0f, -200.0f);
        wall.bounds.center = wall.transform.position;
        wall.bounds.half_extents = Vec2f(500.0f, 10.0f);

        obstacles.emplace_back();
        auto& wall_right = obstacles.back();
        wall_right.transform.position = Vec2f(400.0f, 0.0f);
        wall_right.bounds.center = wall_right.transform.position;
        wall_right.bounds.half_extents = Vec2f(10.0f, 500.0f);

        collectibles.emplace_back();
        collectibles.back().transform.position = Vec2f(100.0f, 50.0f);

        collectibles.emplace_back();
        collectibles.back().transform.position = Vec2f(-80.0f, 30.0f);

        npcs.emplace_back();
        auto& npc = npcs.back();
        npc.transform.position = Vec2f(50.0f, 0.0f);
    }

    void CheckAllCollisions() {
        frame_collisions.clear();

        Collision::AABB player_bounds;
        player_bounds.center = player.transform.position;
        player_bounds.half_extents = Vec2f(15.0f, 20.0f);

        for (auto& enemy : enemies) {
            if (!enemy.health.is_alive) continue;
            Collision::AABB enemy_bounds;
            enemy_bounds.center = enemy.transform.position;
            enemy_bounds.half_extents = Vec2f(15.0f, 20.0f);
            auto info = Collision::ResolveAABB(player_bounds, enemy_bounds);
            if (info.collided) {
                frame_collisions.push_back(info);
            }
        }

        for (auto& obstacle : obstacles) {
            auto info = Collision::ResolveAABB(player_bounds, obstacle.bounds);
            if (info.collided) {
                frame_collisions.push_back(info);
                auto separation = info.penetration_normal * info.penetration_depth;
                player.transform.position = player.transform.position + separation;
            }
        }

        for (auto& proj : projectiles) {
            if (!proj.health.is_alive) continue;
            Collision::Circle proj_circle;
            proj_circle.center = proj.transform.position;
            proj_circle.radius = 5.0f;

            for (auto& enemy : enemies) {
                if (!enemy.health.is_alive) continue;
                Collision::AABB enemy_bounds;
                enemy_bounds.center = enemy.transform.position;
                enemy_bounds.half_extents = Vec2f(15.0f, 20.0f);

                if (enemy_bounds.Contains(proj.transform.position)) {
                    enemy.health.TakeDamage(25.0f);
                    proj.health.is_alive = false;
                    break;
                }
            }
        }

        for (auto& col : collectibles) {
            if (col.tag.tag != TagData::Collectible) continue;
            float dist_sq = Math::LengthSquared(col.transform.position - player.transform.position);
            if (dist_sq <= 20.0f * 20.0f) {
            }
        }
    }

    void Update(float delta_time, RandEngine::Systems::System& system) {
        CheckAllCollisions();
        Perf::BehaviorBPS::Instance().Tick();
    }
};

struct TestEnityScene;

struct TestEnityBenchmarkTickBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override;
};

struct TestEnity;

struct TestEnityGravityBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    PhysicsData* physics = nullptr;
    float gravity_strength = 980.0f;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || !physics || physics->is_kinematic) return;
        transform->acceleration[1] -= gravity_strength;
        transform->Integrate(delta_time);
        transform->acceleration = Vec2f(0.0f, 0.0f);
    }
};

struct TestEnityHealthBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    HealthData* health = nullptr;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!health) return;
        if (health->is_alive && health->current <= 0.0f) {
            health->is_alive = false;
        }
    }
};

struct TestEnityCollisionDataBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    TagData* tag = nullptr;
    Collision::AABB bounds;
    Collision::Circle circle;
    bool use_aabb = true;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform) return;
        if (use_aabb) bounds.center = transform->position;
        else circle.center = transform->position;
    }
};

struct TestEnityPlayerBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    PhysicsData* physics = nullptr;
    Collision::AABB* bounds = nullptr;
    float move_speed = 200.0f;
    float jump_impulse = 400.0f;
    bool grounded = false;
    bool input_left = false;
    bool input_right = false;
    bool input_jump = false;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || !physics || physics->is_kinematic) return;
        Vec2f move_dir{0.0f, 0.0f};
        if (input_left) move_dir[0] -= 1.0f;
        if (input_right) move_dir[0] += 1.0f;
        transform->velocity[0] = move_dir[0] * move_speed;
        if (input_jump && grounded) {
            transform->velocity[1] = jump_impulse;
            grounded = false;
        }
        physics->ApplyDrag(transform->velocity, delta_time);
        transform->Integrate(delta_time);
        if (bounds) bounds->center = transform->position;
    }
};

struct TestEnityEnemyAIBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    PhysicsData* physics = nullptr;
    HealthData* health = nullptr;
    Collision::AABB* bounds = nullptr;
    float chase_speed = 80.0f;
    float detection_range = 250.0f;
    float attack_range = 25.0f;
    float attack_damage = 15.0f;
    float attack_cooldown = 1.0f;
    float attack_timer = 0.0f;
    HealthData* target_health = nullptr;
    TransformData* target_transform = nullptr;

    enum class AIState { Idle, Chase, Attack, Flee };
    AIState ai_state = AIState::Idle;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || !health || !health->is_alive) return;
        attack_timer -= delta_time;
        if (health->Ratio() < 0.2f) ai_state = AIState::Flee;
        switch (ai_state) {
            case AIState::Idle: {
                if (target_transform) {
                    float dist = Math::Length(target_transform->position - transform->position);
                    if (dist <= detection_range) ai_state = AIState::Chase;
                }
                transform->velocity = Vec2f(0.0f, 0.0f);
                break;
            }
            case AIState::Chase: {
                if (!target_transform) { ai_state = AIState::Idle; break; }
                auto diff = target_transform->position - transform->position;
                float dist = Math::Length(diff);
                if (dist > detection_range * 1.5f) { ai_state = AIState::Idle; break; }
                if (dist <= attack_range) { ai_state = AIState::Attack; break; }
                auto dir = diff * (1.0f / dist);
                transform->velocity = dir * chase_speed;
                break;
            }
            case AIState::Attack: {
                if (!target_transform) { ai_state = AIState::Idle; break; }
                float dist = Math::Length(target_transform->position - transform->position);
                if (dist > attack_range * 1.5f) { ai_state = AIState::Chase; break; }
                transform->velocity = Vec2f(0.0f, 0.0f);
                if (attack_timer <= 0.0f && target_health && target_health->is_alive) {
                    target_health->TakeDamage(attack_damage);
                    attack_timer = attack_cooldown;
                }
                break;
            }
            case AIState::Flee: {
                if (!target_transform) { ai_state = AIState::Idle; break; }
                auto diff = transform->position - target_transform->position;
                float dist = Math::Length(diff);
                if (dist > 1e-8f) {
                    auto dir = diff * (1.0f / dist);
                    transform->velocity = dir * chase_speed * 1.2f;
                }
                break;
            }
        }
        if (physics && !physics->is_kinematic) transform->Integrate(delta_time);
        if (bounds) bounds->center = transform->position;
    }
};

struct TestEnityProjectileBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    HealthData* health = nullptr;
    Collision::Circle* circle = nullptr;
    Vec2f projectile_dir{1.0f, 0.0f};
    float projectile_speed = 500.0f;
    float projectile_lifetime = 2.0f;
    float projectile_timer = 0.0f;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || !health || !health->is_alive) return;
        projectile_timer += delta_time;
        if (projectile_timer >= projectile_lifetime) {
            health->is_alive = false;
            return;
        }
        transform->velocity = projectile_dir * projectile_speed;
        transform->Integrate(delta_time);
        if (circle) circle->center = transform->position;
    }
};

struct TestEnityCollectibleBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    float collectible_float_offset = 0.0f;
    float collectible_float_speed = 3.0f;
    float collectible_float_amp = 5.0f;
    float collectible_rot_speed = 2.0f;
    float collectible_rot = 0.0f;
    bool collected = false;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || collected) return;
        collectible_rot += collectible_rot_speed * delta_time;
        float y_offset = std::sin(delta_time * collectible_float_speed * 6.2831853f + collectible_float_offset) * collectible_float_amp;
        transform->position[1] += y_offset;
    }
};

struct TestEnityPatrolBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    std::vector<Vec2f> patrol_points;
    size_t patrol_index = 0;
    float patrol_wait_time = 1.0f;
    float patrol_wait_timer = 0.0f;
    float patrol_speed = 50.0f;
    bool patrol_waiting = false;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || patrol_points.empty()) return;
        if (patrol_waiting) {
            patrol_wait_timer -= delta_time;
            if (patrol_wait_timer <= 0.0f) patrol_waiting = false;
            return;
        }
        auto target = patrol_points[patrol_index];
        auto diff = target - transform->position;
        float dist = Math::Length(diff);
        if (dist <= 5.0f) {
            patrol_index = (patrol_index + 1) % patrol_points.size();
            patrol_waiting = true;
            patrol_wait_timer = patrol_wait_time;
            return;
        }
        auto dir = diff * (1.0f / dist);
        transform->velocity = dir * patrol_speed;
        transform->Integrate(delta_time);
    }
};

struct TestEnityNPCDialogBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    TransformData* target_transform = nullptr;
    float npc_dialog_radius = 50.0f;
    bool npc_dialog_active = false;
    int npc_dialog_line = 0;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || !target_transform) return;
        float dist = Math::Length(target_transform->position - transform->position);
        bool in_range = dist <= npc_dialog_radius;
        if (!in_range && npc_dialog_active) {
            npc_dialog_active = false;
            npc_dialog_line = 0;
        }
    }
};

struct TestEnityTriggerBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    TransformData* player_transform = nullptr;
    float trigger_radius = 60.0f;
    bool trigger_one_shot = false;
    bool trigger_fired = false;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || !player_transform) return;
        if (trigger_one_shot && trigger_fired) return;
        float dist = Math::Length(player_transform->position - transform->position);
        if (dist <= trigger_radius) {
            trigger_fired = true;
        }
    }
};

struct TestEnityOscillationBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    float oscillation_amplitude = 20.0f;
    float oscillation_frequency = 1.0f;
    float oscillation_phase = 0.0f;
    Vec2f oscillation_axis{0.0f, 1.0f};
    Vec2f oscillation_origin{0.0f, 0.0f};

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform) return;
        float offset = std::sin(delta_time * oscillation_frequency * 6.2831853f + oscillation_phase) * oscillation_amplitude;
        transform->position = oscillation_origin + oscillation_axis * offset;
    }
};

struct TestEnityAreaDamageBehavior : virtual Behaviors::BindBaseBehavior, virtual Behaviors::ILogicUpdateBehavior {
    TransformData* transform = nullptr;
    HealthData* health = nullptr;
    float area_damage_radius = 100.0f;
    float area_damage_dps = 10.0f;
    bool area_damage_active = false;

    std::vector<TransformData*> target_transforms;
    std::vector<HealthData*> target_healths;

    void LogicUpdate(float delta_time, RandEngine::Systems::System& system) override {
        Perf::BehaviorBPS::Instance().Record();
        if (!transform || !area_damage_active || !health || !health->is_alive) return;
        for (size_t i = 0; i < target_transforms.size(); ++i) {
            if (!target_healths[i] || !target_healths[i]->is_alive) continue;
            float dist = Math::Length(target_transforms[i]->position - transform->position);
            if (dist <= area_damage_radius) {
                float falloff = 1.0f - (dist / area_damage_radius);
                target_healths[i]->TakeDamage(area_damage_dps * falloff * delta_time);
            }
        }
    }
};

struct TestEnity : virtual BaseObject,
                  virtual BindStaticBehaviors<
                      TestEnityBenchmarkTickBehavior,
                      TestEnityGravityBehavior,
                      TestEnityHealthBehavior,
                      TestEnityCollisionDataBehavior,
                      TestEnityPlayerBehavior,
                      TestEnityEnemyAIBehavior,
                      TestEnityProjectileBehavior,
                      TestEnityCollectibleBehavior,
                      TestEnityPatrolBehavior,
                      TestEnityNPCDialogBehavior,
                      TestEnityTriggerBehavior,
                      TestEnityOscillationBehavior,
                      TestEnityAreaDamageBehavior
                  >,
                  virtual BindDynamicBehaviors<> {
    TransformData transform;
    PhysicsData physics;
    HealthData health;
    TagData tag;
    Collision::AABB bounds;
    Collision::Circle circle;

    bool is_player = false;
    bool is_enemy = false;
    bool is_projectile = false;
    bool is_obstacle = false;
    bool is_collectible = false;
    bool is_npc = false;
    bool is_trigger = false;

    TestEnityGravityBehavior gravity_behavior;
    TestEnityHealthBehavior health_behavior;
    TestEnityCollisionDataBehavior collision_data_behavior;
    TestEnityPlayerBehavior player_behavior;
    TestEnityEnemyAIBehavior enemy_ai_behavior;
    TestEnityProjectileBehavior projectile_behavior;
    TestEnityCollectibleBehavior collectible_behavior;
    TestEnityPatrolBehavior patrol_behavior;
    TestEnityNPCDialogBehavior npc_dialog_behavior;
    TestEnityTriggerBehavior trigger_behavior;
    TestEnityOscillationBehavior oscillation_behavior;
    TestEnityAreaDamageBehavior area_damage_behavior;
    TestEnityBenchmarkTickBehavior bench_tick_behavior;

    TestEnity() {
        gravity_behavior.transform = &transform;
        gravity_behavior.physics = &physics;
        health_behavior.health = &health;
        collision_data_behavior.transform = &transform;
        collision_data_behavior.tag = &tag;
        collision_data_behavior.bounds = bounds;
        collision_data_behavior.circle = circle;
        player_behavior.transform = &transform;
        player_behavior.physics = &physics;
        player_behavior.bounds = &bounds;
        enemy_ai_behavior.transform = &transform;
        enemy_ai_behavior.physics = &physics;
        enemy_ai_behavior.health = &health;
        enemy_ai_behavior.bounds = &bounds;
        projectile_behavior.transform = &transform;
        projectile_behavior.health = &health;
        projectile_behavior.circle = &circle;
        collectible_behavior.transform = &transform;
        patrol_behavior.transform = &transform;
        npc_dialog_behavior.transform = &transform;
        trigger_behavior.transform = &transform;
        oscillation_behavior.transform = &transform;
        area_damage_behavior.transform = &transform;
        area_damage_behavior.health = &health;

        static_behaviors = Behaviors::StaticBehaviors(
            bench_tick_behavior,
            gravity_behavior,
            health_behavior,
            collision_data_behavior,
            player_behavior,
            enemy_ai_behavior,
            projectile_behavior,
            collectible_behavior,
            patrol_behavior,
            npc_dialog_behavior,
            trigger_behavior,
            oscillation_behavior,
            area_damage_behavior
        );
    }

    void SetupAsPlayer() {
        is_player = true;
        tag.tag = TagData::Player;
        tag.layer = 0;
        physics.mass = 1.0f;
        physics.restitution = 0.3f;
        physics.drag = 0.02f;
        health.max_value = 100.0f;
        health.current = 100.0f;
        bounds.half_extents = Vec2f(15.0f, 20.0f);
    }

    void SetupAsEnemy() {
        is_enemy = true;
        tag.tag = TagData::Enemy;
        tag.layer = 1;
        physics.mass = 2.0f;
        physics.restitution = 0.2f;
        physics.drag = 0.05f;
        health.max_value = 60.0f;
        health.current = 60.0f;
        bounds.half_extents = Vec2f(15.0f, 20.0f);
    }

    void SetupAsProjectile(const Vec2f& dir) {
        is_projectile = true;
        tag.tag = TagData::Projectile;
        tag.layer = 2;
        health.max_value = 1.0f;
        health.current = 1.0f;
        projectile_behavior.projectile_dir = dir;
        circle.radius = 5.0f;
    }

    void SetupAsObstacle(const Vec2f& half_ext) {
        is_obstacle = true;
        tag.tag = TagData::Obstacle;
        tag.layer = 3;
        physics.is_kinematic = true;
        physics.mass = 0.0f;
        physics.restitution = 1.0f;
        bounds.half_extents = half_ext;
    }

    void SetupAsCollectible() {
        is_collectible = true;
        tag.tag = TagData::Collectible;
        tag.layer = 4;
        collectible_behavior.collectible_float_offset = static_cast<float>(rand()) / RAND_MAX * 6.28f;
    }

    void SetupAsNPC() {
        is_npc = true;
        tag.tag = TagData::NPC;
        tag.layer = 5;
    }

    void SetupAsTrigger(float radius, bool one_shot = false) {
        is_trigger = true;
        tag.tag = TagData::Trigger;
        tag.layer = 6;
        trigger_behavior.trigger_radius = radius;
        trigger_behavior.trigger_one_shot = one_shot;
    }

    void SetupPatrol(const std::vector<Vec2f>& points) {
        patrol_behavior.patrol_points = points;
        patrol_behavior.patrol_index = 0;
        patrol_behavior.patrol_waiting = false;
    }

    void SetupOscillation(const Vec2f& axis, float amp, float freq, float phase = 0.0f) {
        oscillation_behavior.oscillation_axis = axis;
        oscillation_behavior.oscillation_amplitude = amp;
        oscillation_behavior.oscillation_frequency = freq;
        oscillation_behavior.oscillation_phase = phase;
        oscillation_behavior.oscillation_origin = transform.position;
    }

    void SetupAreaDamage(float radius, float dps) {
        area_damage_behavior.area_damage_radius = radius;
        area_damage_behavior.area_damage_dps = dps;
        area_damage_behavior.area_damage_active = true;
    }
};

struct TestEnitySpinLock {
    std::atomic<bool> flag{false};

    void Lock() noexcept {
        while (flag.exchange(true, std::memory_order_acquire)) {
            #if defined(_MSC_VER)
            _mm_pause();
            #else
            __builtin_ia32_pause();
            #endif
        }
    }

    void Unlock() noexcept {
        flag.store(false, std::memory_order_release);
    }
};

struct TestEnityScopedSpinLock {
    TestEnitySpinLock& lk;
    TestEnityScopedSpinLock(TestEnitySpinLock& l) noexcept : lk(l) { lk.Lock(); }
    ~TestEnityScopedSpinLock() noexcept { lk.Unlock(); }
};

struct TestEnityScene {
    static inline std::vector<Collision::CollisionInfo> frame_collisions;
    static inline TestEnitySpinLock output_lock;

    static void ResolveCollisions(TestEnity* entities, size_t count) {
        frame_collisions.clear();

        for (size_t i = 0; i < count; ++i) {
            if (!entities[i].health.is_alive) continue;
            for (size_t j = i + 1; j < count; ++j) {
                if (!entities[j].health.is_alive) continue;

                bool can_collide = false;
                if (entities[i].is_player && entities[j].is_enemy) can_collide = true;
                else if (entities[i].is_enemy && entities[j].is_player) can_collide = true;
                else if (entities[i].is_player && entities[j].is_obstacle) can_collide = true;
                else if (entities[i].is_obstacle && entities[j].is_player) can_collide = true;
                else if (entities[i].is_projectile && entities[j].is_enemy) can_collide = true;
                else if (entities[i].is_enemy && entities[j].is_projectile) can_collide = true;
                else if (entities[i].is_collectible && entities[j].is_player) can_collide = true;
                else if (entities[i].is_player && entities[j].is_collectible) can_collide = true;
                else if (entities[i].is_trigger && entities[j].is_player) can_collide = true;
                else if (entities[i].is_player && entities[j].is_trigger) can_collide = true;

                if (!can_collide) continue;

                if (entities[i].is_projectile || entities[j].is_projectile) {
                    auto& proj = entities[i].is_projectile ? entities[i] : entities[j];
                    auto& other = entities[i].is_projectile ? entities[j] : entities[i];
                    if (other.bounds.Contains(proj.transform.position)) {
                        if (other.is_enemy) other.health.TakeDamage(25.0f);
                        proj.health.is_alive = false;
                        auto info = Collision::ResolveAABB(other.bounds, other.bounds);
                        info.collided = true;
                        frame_collisions.push_back(info);
                    }
                } else if (entities[i].is_obstacle || entities[j].is_obstacle) {
                    auto& obstacle = entities[i].is_obstacle ? entities[i] : entities[j];
                    auto& other = entities[i].is_obstacle ? entities[j] : entities[i];
                    auto info = Collision::ResolveAABB(other.bounds, obstacle.bounds);
                    if (info.collided) {
                        frame_collisions.push_back(info);
                        auto separation = info.penetration_normal * info.penetration_depth;
                        other.transform.position = other.transform.position + separation;
                    }
                } else {
                    auto info = Collision::ResolveAABB(entities[i].bounds, entities[j].bounds);
                    if (info.collided) {
                        frame_collisions.push_back(info);
                    }
                }
            }
        }
    }

    static void UpdateTriggers(TestEnity* entities, size_t count) {
        TestEnity* player = nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (entities[i].is_player) { player = &entities[i]; break; }
        }
        if (!player) return;
        for (size_t i = 0; i < count; ++i) {
            if (!entities[i].is_trigger) continue;
            entities[i].trigger_behavior.player_transform = &player->transform;
        }
    }

    static void UpdateAreaDamages(TestEnity* entities, size_t count, float delta_time) {
        for (size_t i = 0; i < count; ++i) {
            if (!entities[i].area_damage_behavior.area_damage_active) continue;
            entities[i].area_damage_behavior.target_transforms.clear();
            entities[i].area_damage_behavior.target_healths.clear();
            for (size_t j = 0; j < count; ++j) {
                if (i == j) continue;
                entities[i].area_damage_behavior.target_transforms.push_back(&entities[j].transform);
                entities[i].area_damage_behavior.target_healths.push_back(&entities[j].health);
            }
        }
    }
};

inline void TestEnityBenchmarkTickBehavior::LogicUpdate(float delta_time, RandEngine::Systems::System& system) {
    Perf::BehaviorBPS::Instance().Record();
    Perf::BehaviorBPS::Instance().Tick();
}

} // namespace RandEngine::Core::Objects