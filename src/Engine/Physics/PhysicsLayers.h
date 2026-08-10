#pragma once

// Jolt Physics collision layers and filters.
// Kept in a separate header so PhysicsSystem.cpp doesn't pollute other TUs.

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

JPH_SUPPRESS_WARNINGS

namespace Lgt::Physics {

/// Object layers — which logical group a body belongs to.
namespace Layers {
    static constexpr JPH::ObjectLayer STATIC  = 0;
    static constexpr JPH::ObjectLayer DYNAMIC = 1;
    static constexpr JPH::ObjectLayer NUM     = 2;
}

/// Broad-phase layers — separate BVH trees for static vs dynamic.
namespace BPLayers {
    static constexpr JPH::BroadPhaseLayer STATIC(0);
    static constexpr JPH::BroadPhaseLayer DYNAMIC(1);
    static constexpr unsigned int NUM = 2;
}

/// Maps object layers → broad-phase layers.
class BPLayerInterface final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterface() {
        _objectToBP[Layers::STATIC]  = BPLayers::STATIC;
        _objectToBP[Layers::DYNAMIC] = BPLayers::DYNAMIC;
    }

    JPH::uint GetNumBroadPhaseLayers() const override { return BPLayers::NUM; }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM);
        return _objectToBP[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
        case (JPH::BroadPhaseLayer::Type)BPLayers::STATIC:  return "STATIC";
        case (JPH::BroadPhaseLayer::Type)BPLayers::DYNAMIC: return "DYNAMIC";
        default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer _objectToBP[Layers::NUM];
};

/// Determines which object layer pairs can collide.
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inObj1, JPH::ObjectLayer inObj2) const override {
        switch (inObj1) {
        case Layers::STATIC:  return inObj2 == Layers::DYNAMIC;
        case Layers::DYNAMIC: return true;
        default: JPH_ASSERT(false); return false;
        }
    }
};

/// Determines which object layers can collide with which broad-phase layers.
class ObjectVsBPLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
        case Layers::STATIC:  return inLayer2 == BPLayers::DYNAMIC;
        case Layers::DYNAMIC: return true;
        default: JPH_ASSERT(false); return false;
        }
    }
};

} // namespace Lgt::Physics
