#include "Player.h"

namespace Game::System {
void Player::Init(const ApplicationContext& appContext, PlayerContext& playerContext) {

    auto player = appContext.world->CreateEntity("Player");

    player.Add<Lgt::Component::RigidBody>().motionType = Lgt::Component::MotionType::Dynamic;
    player.Add<Lgt::Component::CapsuleCollider>(1.0f, 0.2f);
    player.Add<Lgt::Component::Camera>();
    auto sphere = appContext.assets->LoadModel("D:/DEV/cpp/LightVK/Assets/Sphere/Untitled.gltf");
    if (sphere)
        player.Add<Lgt::Component::ModelInstance>().model = sphere;

    player.Add<Component::Player>("Mahesh", 100, true);

    auto& transform = player.Get<Lgt::Component::LocalTransform>();

    transform.position    = glm::vec3(0.0f, 2.0f, 5.0f); // Move up and back so we aren't clipping the floor
    transform.scale      *= 0.1f;
    playerContext.player  = player;
}

void Player::Update(float dt, const ApplicationContext& appContext, PlayerContext& playerContext) {
    auto& player  = playerContext.player;
    auto& camera  = player.Get<Lgt::Component::Camera>();
    auto& physics = appContext.world->GetPhysics();

    glm::vec3 currentVel = physics.GetLinearVelocity(player);

    glm::vec3 moveVel(0.0f);
    float     speed = 5.0f;

    if (appContext.input->IsKeyDown(Lgt::Key::LeftShift))
        speed *= 2.0f;

    glm::vec3 right = glm::normalize(glm::cross(camera.front, camera.up));

    if (appContext.input->IsKeyDown(Lgt::Key::W))
        moveVel += camera.front;

    if (appContext.input->IsKeyDown(Lgt::Key::S))
        moveVel -= camera.front;

    if (appContext.input->IsKeyDown(Lgt::Key::A))
        moveVel -= right;

    if (appContext.input->IsKeyDown(Lgt::Key::D))
        moveVel += right;

    if (appContext.input->IsKeyDown(Lgt::Key::E))
        moveVel += camera.up;

    if (appContext.input->IsKeyDown(Lgt::Key::Q))
        moveVel -= camera.up;

    // Prevent diagonal movement from being faster
    if (glm::length2(moveVel) > 0.0f)
        moveVel = glm::normalize(moveVel) * speed;

    // Preserve physics velocity, e.g. gravity
    moveVel.y = currentVel.y;

    physics.SetLinearVelocity(player, moveVel);
}

} // namespace Game::System
