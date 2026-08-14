#include "Transform.h"
#include "Engine/Core/Logger.h"
#include "Engine/Scene/Components.h"

namespace Lgt {

System::Transform::Transform(World* world) {
    LGT_ASSERT(world, "");
    _world = world;
}

void System::Transform::Update() {

    const auto& view = _world->Registry().view<Component::Hierarchy, Component::WorldTransform, Component::LocalTransform>();

    for (auto e : view) {
        Entity entity(e, _world);
        auto&  entity_h = entity.Get<Component::Hierarchy>();

        if (!entity_h.parent)
            UpdateSubtree(entity);
    }
}

void System::Transform::ComputeWorld(Entity entity) {
    if (!entity.IsValid())
        return;

    auto& entity_h       = entity.Get<Component::Hierarchy>();
    auto& entity_world_t = entity.Get<Component::WorldTransform>();
    auto& entity_local_t = entity.Get<Component::LocalTransform>();

    if (!entity_h.parent) {
        entity_world_t.matrix = entity_local_t.Matrix();
        return;
    } else {
        auto& parent_world_t  = entity_h.parent.Get<Component::WorldTransform>();
        entity_world_t.matrix = parent_world_t.matrix * entity_local_t.Matrix();
    }
}

void System::Transform::UpdateSubtree(Entity entity) {

    if (!entity)
        return;

    ComputeWorld(entity);

    auto& entity_h = entity.Get<Component::Hierarchy>();
    auto  next     = entity_h.firstChild;

    while (next) {
        UpdateSubtree(next);
        next = next.Get<Component::Hierarchy>().nextSibling;
    }
}

} // namespace  Lgt
