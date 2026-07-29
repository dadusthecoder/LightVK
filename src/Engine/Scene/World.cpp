
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/World.h"
#include "Engine/Scene/Components.h"

#include "Engine/Renderer/Gpu/Renderer.h"
#include "Engine/Scene/Systems/Transform.h"

namespace Lgt {

World::World()
    : transform_sys(this) {}

Entity World::CreateEntity(std::string name) {
    auto   handle = m_Registry.create();
    Entity e(handle, this);

    // every entity must have this components;
    e.Add<Component::Tag>(name);
    e.Add<Component::Hierarchy>();
    e.Add<Component::WorldTransform>();
    e.Add<Component::LocalTransform>();

    return e;
}

void World::DestroyEntity(Entity entity) {
    m_Registry.destroy(entity.Handle());
}

void World::Update(float /*deltaTime*/) {
    transform_sys.Update();
}

Gpu::DrawList World::DrawList() {
    return Gpu::DrawList();
}

} // namespace Lgt