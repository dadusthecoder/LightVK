#include "Player.h"

namespace Game {
void PlayerSystem::Init(Lgt::World* world) {

    player = world->CreateEntity("Player");
    player.Add<Components::Player>("Mahesh", 100, true);
    player.Add<Lgt::Component::Camera>();
}

void PlayerSystem::Update(float dt, Lgt::InputManager* input, Lgt::World* world) {
    auto& transform = player.Get<Lgt::Component::LocalTransform>();
    
    if (input->IsKeyDown(Lgt::Key::W)) {
        transform.position.y += 0.1f;
    }   
    if (input->IsKeyDown(Lgt::Key::S)) {
        transform.position.y -= 0.1f;
    }
    if (input->IsKeyDown(Lgt::Key::A)) {
        transform.position.x += 0.1f;
    }
    if (input->IsKeyDown(Lgt::Key::D)) {
        transform.position.x -= 0.1f;
    }
}

} // namespace Game
