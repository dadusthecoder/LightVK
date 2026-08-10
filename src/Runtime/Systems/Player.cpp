#include "Player.h"

namespace Game::System {
void Player::Init(const ApplicationContext& appContext, PlayerContext& playerContext) {

    auto player = appContext.world->CreateEntity("Player");

    player.Add<Lgt::Component::RigidBody>().motionType = Lgt::Component::MotionType::Kinematic;
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
    auto& transform = playerContext.player.Get<Lgt::Component::LocalTransform>();

    if (appContext.input->IsKeyDown(Lgt::Key::W))
        transform.position.z -= 1.0f * dt;
    if (appContext.input->IsKeyDown(Lgt::Key::S))
        transform.position.z += 1.0f * dt;
    if (appContext.input->IsKeyDown(Lgt::Key::A))
        transform.position.x -= 1.0f * dt;
    if (appContext.input->IsKeyDown(Lgt::Key::D))
        transform.position.x += 1.0f * dt;
    if (appContext.input->IsKeyDown(Lgt::Key::Space))
        transform.position.y += 1.0f * dt;
    if (appContext.input->IsKeyDown(Lgt::Key::LeftControl))
        transform.position.y -= 1.0f * dt;
}

} // namespace Game::System
