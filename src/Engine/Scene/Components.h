#pragma once
#include <string>
#include "Engine/Scene/Entity.h"
#include "Engine/Assets/AssetTypes.h"
#include "Engine/Physics/PhysicsComponents.h"
#include <glm/glm.hpp>

// REQUIRED extensions
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Lgt::Component {

struct Tag {
    std::string name = "Entity";
};

struct Hierarchy {
    Entity parent      = Entity::Null();
    Entity firstChild  = Entity::Null();
    Entity nextSibling = Entity::Null();
    Entity prevSibling = Entity::Null();
};

struct WorldTransform {
    glm::mat4 matrix = glm::mat4(1.0f);
};

struct LocalTransform {
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale    = {1.f, 1.f, 1.f};

    glm::mat4 Matrix() const {
        glm::mat4 t = glm::translate(glm::mat4(1.f), position);
        glm::mat4 r = glm::mat4_cast(rotation);
        glm::mat4 s = glm::scale(glm::mat4(1.f), scale);
        return t * r * s;
    }
};

struct Material {
    glm::vec4 albedo           = {1.f, 1.f, 1.f, 1.f};
    float     roughness        = 0.5f;
    float     metallic         = 0.0f;
    float     emissiveStrength = 0.0f;
};

struct DirectionalLight {
    glm::vec3 direction = glm::normalize(glm::vec3(-0.5f, -1.f, -0.8f));
    glm::vec3 color     = {1.f, 1.f, 1.f};
    float     intensity = 1.f;
};

struct PointLight {
    glm::vec3 color     = {1.f, 1.f, 1.f};
    float     intensity = 1.f;
    float     radius    = 10.f;
};

struct ModelInstance {
    Lgt::Assets::AssetGuid model{};
    Lgt::Assets::Mobility  mobility      = Lgt::Assets::Mobility::Static;
    bool                   visible       = true;
    bool                   castShadow    = true;
    bool                   receiveShadow = true;
};

struct Camera {
    glm::vec3 up    = {0.f, 1.f, 0.f};
    glm::vec3 front = {0.f, 0.f, -1.f};

    float fov       = 45.0f; // degrees
    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;
    bool  isActive  = true; // only the active camera is used for rendering

    /// Computes the view matrix from the entity's world position.
    glm::mat4 ViewMatrix(const glm::vec3& worldPos) const { return glm::lookAt(worldPos, worldPos + front, up); }

    /// Computes the perspective projection matrix (Vulkan Y-flipped).
    glm::mat4 ProjectionMatrix(float aspect) const {
        auto proj   = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
        proj[1][1] *= -1.0f; // Vulkan clip-space Y is inverted
        return proj;
    }
};

} // namespace Lgt::Component
