#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Lgt::Component {

/// Physics motion type — determines how Jolt treats the body.
enum class MotionType : uint8_t {
    Static,     // Never moves (floors, walls)
    Dynamic,    // Fully simulated by physics
    Kinematic   // Moved by code, pushes dynamics
};

/// Collision shape type tag (which collider component to look for).
enum class ShapeType : uint8_t {
    None,
    Box,
    Sphere,
    Capsule
};

/// Core rigid body component — attach this + a collider to make an entity physical.
struct RigidBody {
    MotionType motionType    = MotionType::Dynamic;
    float      mass          = 1.0f;
    float      linearDamping = 0.05f;
    float      angularDamping = 0.05f;
    float      friction      = 0.5f;
    float      restitution   = 0.3f;
    float      gravityFactor = 1.0f;

    // Runtime state — set by PhysicsSystem, do not edit manually
    uint32_t   bodyId        = 0xFFFFFFFF; // JPH::BodyID::cInvalidBodyID
    bool       _registered   = false;
};

/// Axis-aligned box collider.
struct BoxCollider {
    glm::vec3 halfExtents = {0.5f, 0.5f, 0.5f};
};

/// Sphere collider.
struct SphereCollider {
    float radius = 0.5f;
};

/// Capsule collider (Y-axis aligned).
struct CapsuleCollider {
    float halfHeight = 0.5f;
    float radius     = 0.25f;
};

} // namespace Lgt::Component
