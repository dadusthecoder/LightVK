#pragma once

#include <glm/vec3.hpp>
#include <vector>
#include <cstdint>

// Lightweight physics query API stubs for the engine.
// These are design-first headers to integrate with the existing PhysicsSystem (Jolt-based).
// Implementations should live in PhysicsSystem.cpp or a dedicated PhysicsQueries.cpp and use
// the engine's chosen collision/physics backend.

namespace Lgt {
    class Entity;
}

namespace Lgt::Physics {

struct RaycastHit {
    Lgt::Entity hitEntity = Lgt::Entity(); // default / invalid
    glm::vec3 point{};      // world-space impact point
    glm::vec3 normal{};     // surface normal at impact (unit)
    float distance = 0.0f;  // distance from ray origin
    int triangleIndex = -1; // optional for triangle meshes
};

struct SweepHit {
    Lgt::Entity hitEntity = Lgt::Entity();
    glm::vec3 point{};
    glm::vec3 normal{};
    float timeOfImpact = 0.0f; // fraction along sweep (0..1)
};

struct ContactPoint {
    glm::vec3 point;
    glm::vec3 normal;
    float penetrationDepth;
};

struct ContactManifold {
    Lgt::Entity a, b;
    std::vector<ContactPoint> points;
};

// Raycast: cast a ray and return first hit. Returns true if a hit was found.
bool Raycast(const glm::vec3& origin,
             const glm::vec3& direction,
             float maxDistance,
             RaycastHit& outHit,
             uint32_t layerMask = 0xFFFFFFFFu);

// Capsule sweep: sweep a capsule along delta (world-space). Returns true if any hit occurs.
// The capsule is defined by two points (p0, p1) and a radius.
bool SweepCapsule(const glm::vec3& p0,
                  const glm::vec3& p1,
                  float radius,
                  const glm::vec3& delta,
                  SweepHit& outHit,
                  uint32_t layerMask = 0xFFFFFFFFu);

// Overlap: return all entities overlapping a sphere at center with given radius.
std::vector<Lgt::Entity> OverlapSphere(const glm::vec3& center,
                                       float radius,
                                       uint32_t layerMask = 0xFFFFFFFFu);

// Retrieve contact manifold between two entities if they are in contact.
// Returns true if a manifold was filled.
bool GetContactManifold(Lgt::Entity a, Lgt::Entity b, ContactManifold& outManifold);

// Predict if entity will collide within dt when moving with given velocity. Useful for gameplay logic.
bool PredictCollision(Lgt::Entity entity, const glm::vec3& velocity, float dt, SweepHit& outHit);

} // namespace Lgt::Physics
