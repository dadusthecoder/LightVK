#pragma once

#include <memory>
#include <glm/glm.hpp>

// Forward declarations — avoid pulling Jolt into every TU
namespace JPH {
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
}

namespace Lgt {
class World;
class Entity;
} // namespace Lgt

namespace Lgt::System {

/// Jolt Physics system — manages simulation, body creation, and ECS sync.
class Physics {
public:
    explicit Physics(World* world);
    ~Physics();

    /// One-time setup. Call after World is fully constructed.
    void Init();

    /// Step the simulation and sync dynamic body poses back to the ECS.
    void Update(float deltaTime);

    /// Cleanup Jolt resources. Call before World is destroyed.
    void Shutdown();

    /// Scans the ECS for new RigidBody+Collider combos and registers them with Jolt.
    void SyncNewBodies();

    /// Removes a specific body from the physics world.
    void RemoveBody(Entity entity);

    /// Push an entity's LocalTransform into Jolt (useful for kinematic/editor moves).
    void SyncToPhysics(Entity entity);

    void      SetLinearVelocity(Entity entity, const glm::vec3& velocity);
    glm::vec3 GetLinearVelocity(Entity entity);

private:
    World* _world = nullptr;

    // Jolt core objects (opaque pointers to avoid Jolt headers in every TU)
    std::unique_ptr<JPH::PhysicsSystem>      _physicsSystem;
    std::unique_ptr<JPH::TempAllocatorImpl>   _tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> _jobSystem;

    // Layer filters — small POD objects, stored inline
    struct Impl;
    std::unique_ptr<Impl> _impl;

    bool _initialized = false;
};

} // namespace Lgt::System
