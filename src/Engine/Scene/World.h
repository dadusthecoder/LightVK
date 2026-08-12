#pragma once

#include <functional>
#include "Engine/Scene/SceneGraph.h"
#include "Engine/Core/GlmConfig.h"
#include "Engine/Scene/Systems/Transform.h"
#include "Engine/Physics/PhysicsSystem.h"

#include <entt/entt.hpp>

namespace Lgt {

class World {
public:
    explicit World();
    ~World();

    Entity CreateEntity(std::string name = "Entity");
    void   DestroyEntity(Entity entity);
    void   Update(float deltaTime);

    entt::registry&       Registry() { return m_Registry; }
    const entt::registry& Registry() const { return m_Registry; }
    SceneGraph&           Graph() { return _graph; }
    System::Physics&      GetPhysics() { return _physics_sys; }

    template <typename... Components> auto GetView() { return m_Registry.view<Components...>(); }

private:
    System::Transform transform_sys;
    System::Physics   _physics_sys;
    entt::registry    m_Registry;
    SceneGraph        _graph;
};

} // namespace Lgt