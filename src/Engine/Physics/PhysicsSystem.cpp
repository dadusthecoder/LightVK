// Jolt.h MUST be included before any other Jolt header.
#include <Jolt/Jolt.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

JPH_SUPPRESS_WARNINGS

#include "PhysicsSystem.h"
#include "PhysicsComponents.h"
#include "PhysicsLayers.h"

#include "Engine/Scene/World.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"
#include "Engine/Core/Logger.h"

#include <thread>

using namespace JPH;
using namespace JPH::literals;

// ─── Jolt trace / assert callbacks ──────────────────────────────────────────

static void JoltTraceImpl(const char* inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    LIGHTVK_TRACE("[Jolt] {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    LIGHTVK_ERROR("[Jolt] Assert: {} : {} ({}:{})", inExpression, inMessage ? inMessage : "", inFile, inLine);
    return true; // break into debugger
}
#endif

namespace Lgt::System {

// ─── Impl (holds Jolt filter objects) ───────────────────────────────────────

struct Physics::Impl {
    Impl() = default;

    Lgt::Physics::BPLayerInterface            _bpLayerInterface;
    Lgt::Physics::ObjectVsBPLayerFilterImpl    _objectVsBP;
    Lgt::Physics::ObjectLayerPairFilterImpl    _objectLayerPair;
};

// ─── Helpers ────────────────────────────────────────────────────────────────

static inline JPH::RVec3 ToJolt(const glm::vec3& v) {
    return JPH::RVec3(v.x, v.y, v.z);
}

static inline JPH::Quat ToJolt(const glm::quat& q) {
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

static inline glm::vec3 ToGlm(const JPH::RVec3& v) {
    return glm::vec3(static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()));
}

static inline glm::quat ToGlm(const JPH::Quat& q) {
    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

static EMotionType ToJoltMotion(Component::MotionType mt) {
    switch (mt) {
    case Component::MotionType::Static:    return EMotionType::Static;
    case Component::MotionType::Dynamic:   return EMotionType::Dynamic;
    case Component::MotionType::Kinematic: return EMotionType::Kinematic;
    default: return EMotionType::Static;
    }
}

static ObjectLayer GetObjectLayer(Component::MotionType mt) {
    return (mt == Component::MotionType::Static)
        ? Lgt::Physics::Layers::STATIC
        : Lgt::Physics::Layers::DYNAMIC;
}

// ─── Constructor / Destructor ───────────────────────────────────────────────

Physics::Physics(World* world)
    : _world(world)
    , _impl(std::make_unique<Impl>()) {}

Physics::~Physics() {
    if (_initialized)
        Shutdown();
}

// ─── Init ───────────────────────────────────────────────────────────────────

void Physics::Init() {
    if (_initialized) return;

    // Register Jolt global state (only once per process)
    JPH::RegisterDefaultAllocator();
    JPH::Trace = JoltTraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = JoltAssertFailedImpl;
#endif

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    // Temp allocator — 10 MB
    _tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // Job system — use hardware thread count minus 1 (main thread does work too)
    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
    _jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, static_cast<int>(numThreads)
    );

    // Physics system
    constexpr uint cMaxBodies      = 4096;
    constexpr uint cNumBodyMutexes = 0; // default
    constexpr uint cMaxBodyPairs   = 4096;
    constexpr uint cMaxContactConstraints = 2048;

    _physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    _physicsSystem->Init(
        cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
        _impl->_bpLayerInterface, _impl->_objectVsBP, _impl->_objectLayerPair
    );

    // Set default gravity
    _physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    _initialized = true;
    LIGHTVK_INFO("[Physics] Jolt Physics initialized ({} worker threads)", numThreads);
}

// ─── Shutdown ───────────────────────────────────────────────────────────────

void Physics::Shutdown() {
    if (!_initialized) return;

    // Remove all bodies from the physics world
    auto& reg = _world->Registry();
    auto view = reg.view<Component::RigidBody>();
    for (auto e : view) {
        auto& rb = view.get<Component::RigidBody>(e);
        if (rb._registered && rb.bodyId != 0xFFFFFFFF) {
            JPH::BodyID id(rb.bodyId);
            auto& bi = _physicsSystem->GetBodyInterface();
            bi.RemoveBody(id);
            bi.DestroyBody(id);
            rb._registered = false;
            rb.bodyId = 0xFFFFFFFF;
        }
    }

    _physicsSystem.reset();
    _jobSystem.reset();
    _tempAllocator.reset();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    _initialized = false;
    LIGHTVK_INFO("[Physics] Jolt Physics shut down");
}

// ─── SyncNewBodies ──────────────────────────────────────────────────────────

void Physics::SyncNewBodies() {
    if (!_initialized) return;

    auto& reg = _world->Registry();
    auto& bi  = _physicsSystem->GetBodyInterface();

    // Scan for RigidBody components that haven't been registered
    auto view = reg.view<Component::RigidBody, Component::LocalTransform>();
    for (auto e : view) {
        auto& rb = view.get<Component::RigidBody>(e);
        if (rb._registered) continue;

        auto& lt = view.get<Component::LocalTransform>(e);

        // Determine shape from attached collider
        JPH::ShapeRefC shape;

        if (reg.all_of<Component::BoxCollider>(e)) {
            auto& col = reg.get<Component::BoxCollider>(e);
            shape = new JPH::BoxShape(JPH::Vec3(col.halfExtents.x, col.halfExtents.y, col.halfExtents.z));
        } else if (reg.all_of<Component::SphereCollider>(e)) {
            auto& col = reg.get<Component::SphereCollider>(e);
            shape = new JPH::SphereShape(col.radius);
        } else if (reg.all_of<Component::CapsuleCollider>(e)) {
            auto& col = reg.get<Component::CapsuleCollider>(e);
            shape = new JPH::CapsuleShape(col.halfHeight, col.radius);
        } else {
            // No collider attached — skip
            continue;
        }

        // Create body
        JPH::BodyCreationSettings settings(
            shape,
            ToJolt(lt.position),
            ToJolt(lt.rotation),
            ToJoltMotion(rb.motionType),
            GetObjectLayer(rb.motionType)
        );

        settings.mFriction       = rb.friction;
        settings.mRestitution    = rb.restitution;
        settings.mLinearDamping  = rb.linearDamping;
        settings.mAngularDamping = rb.angularDamping;
        settings.mGravityFactor  = rb.gravityFactor;

        if (rb.motionType == Component::MotionType::Dynamic) {
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            settings.mMassPropertiesOverride.mMass = rb.mass;
        }

        JPH::Body* body = bi.CreateBody(settings);
        if (!body) {
            LIGHTVK_ERROR("[Physics] Failed to create body — max bodies exceeded?");
            continue;
        }

        bi.AddBody(body->GetID(),
            (rb.motionType == Component::MotionType::Static)
                ? JPH::EActivation::DontActivate
                : JPH::EActivation::Activate
        );

        rb.bodyId = body->GetID().GetIndexAndSequenceNumber();
        rb._registered = true;
    }
}

// ─── RemoveBody ─────────────────────────────────────────────────────────────

void Physics::RemoveBody(Entity entity) {
    if (!_initialized || !entity.Has<Component::RigidBody>()) return;

    auto& rb = entity.Get<Component::RigidBody>();
    if (!rb._registered || rb.bodyId == 0xFFFFFFFF) return;

    JPH::BodyID id(rb.bodyId);
    auto& bi = _physicsSystem->GetBodyInterface();
    bi.RemoveBody(id);
    bi.DestroyBody(id);

    rb._registered = false;
    rb.bodyId = 0xFFFFFFFF;
}

// ─── SyncToPhysics ──────────────────────────────────────────────────────────

void Physics::SyncToPhysics(Entity entity) {
    if (!_initialized) return;
    if (!entity.Has<Component::RigidBody>() || !entity.Has<Component::LocalTransform>()) return;

    auto& rb = entity.Get<Component::RigidBody>();
    if (!rb._registered || rb.bodyId == 0xFFFFFFFF) return;

    auto& lt = entity.Get<Component::LocalTransform>();
    JPH::BodyID id(rb.bodyId);
    auto& bi = _physicsSystem->GetBodyInterface();

    bi.SetPositionAndRotation(id, ToJolt(lt.position), ToJolt(lt.rotation), JPH::EActivation::Activate);
}

void Physics::SetLinearVelocity(Entity entity, const glm::vec3& velocity) {
    if (!_initialized || !entity.Has<Component::RigidBody>()) return;
    auto& rb = entity.Get<Component::RigidBody>();
    if (!rb._registered || rb.bodyId == 0xFFFFFFFF) return;
    
    JPH::BodyID id(rb.bodyId);
    _physicsSystem->GetBodyInterface().SetLinearVelocity(id, ToJolt(velocity));
}

glm::vec3 Physics::GetLinearVelocity(Entity entity) {
    if (!_initialized || !entity.Has<Component::RigidBody>()) return glm::vec3(0.0f);
    auto& rb = entity.Get<Component::RigidBody>();
    if (!rb._registered || rb.bodyId == 0xFFFFFFFF) return glm::vec3(0.0f);
    
    JPH::BodyID id(rb.bodyId);
    return ToGlm(_physicsSystem->GetBodyInterface().GetLinearVelocity(id));
}

// ─── Update ─────────────────────────────────────────────────────────────────

void Physics::Update(float deltaTime) {
    if (!_initialized) return;

    // Register any new bodies added via ECS
    SyncNewBodies();

    // Step physics
    constexpr int cCollisionSteps = 1;
    _physicsSystem->Update(deltaTime, cCollisionSteps, _tempAllocator.get(), _jobSystem.get());

    // Sync physics poses back to ECS (dynamic bodies only)
    auto& reg = _world->Registry();
    auto& bi  = _physicsSystem->GetBodyInterface();

    auto view = reg.view<Component::RigidBody, Component::LocalTransform>();
    for (auto e : view) {
        auto& rb = view.get<Component::RigidBody>(e);
        if (!rb._registered || rb.bodyId == 0xFFFFFFFF) continue;
        if (rb.motionType == Component::MotionType::Static) continue;

        JPH::BodyID id(rb.bodyId);

        JPH::RVec3 pos;
        JPH::Quat  rot;
        bi.GetPositionAndRotation(id, pos, rot);

        auto& lt = view.get<Component::LocalTransform>(e);
        lt.position = ToGlm(pos);
        lt.rotation = ToGlm(rot);
    }
}

} // namespace Lgt::System
