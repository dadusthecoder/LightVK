#include "PlayerCamera.h"

#include "../Context.h"
#include "Engine/Renderer/Gpu/Context.h"

namespace Game::System {
void PlayerCamera::Init(const ApplicationContext& appContext, PlayerContext& playerContext) {}
void PlayerCamera::Shutdown() {}

float yaw   = -90.0f;
float pitch = 0.0f;

void PlayerCamera::Update(float dt, const ApplicationContext& appContext, PlayerContext& playerContext) {

    auto* input           = appContext.input;
    auto& playerTransform = playerContext.player.Get<Lgt::Component::LocalTransform>();
    auto  camera          = playerContext.player.Get<Lgt::Component::Camera>();

    if (!input->IsCursorCaptured() && input->IsMouseDown(Lgt::Mouse::Left))
        input->SetCursorCaptured(true);
    else if (input->IsKeyDown(Lgt::Key::Escape))
        input->SetCursorCaptured(false);

    if (input->IsCursorCaptured()) {

        glm::vec2 mouseDelta = input->GetMouseDelta();

        yaw   += mouseDelta.x * 0.1f;
        pitch -= mouseDelta.y * 0.1f;

        pitch = glm::clamp(pitch, -80.0f, 80.0f);
    }

    // Direction from player -> camera orbit direction
    glm::vec3 direction;

    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    direction = glm::normalize(direction);

    // Point camera at player's upper body/head
    glm::vec3 target = playerTransform.position + glm::vec3(0.0f, 0.15f, 0.0f);

    // Camera sits behind target
    float cameraDistance = 0.5f;

    glm::vec3 cameraPos = target - direction * cameraDistance;

    // camera. = cameraPos;
    camera.front = glm::normalize(target - cameraPos);

    playerContext.camera = camera.ProjectionMatrix(800.0f / 600.0f) * camera.ViewMatrix(cameraPos);
}

} // namespace Game::System
