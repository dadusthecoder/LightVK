#pragma once
#include "Engine/Core/LightVK.h"

namespace Game {

struct ApplicationContext {
    Lgt::World*                world  = nullptr;
    Lgt::InputManager*         input  = nullptr;
    Lgt::Timer*                timer  = nullptr;
    Lgt::Assets::AssetManager* assets = nullptr;
};

struct PlayerContext {
    bool        isCursorLocked = false;
    Lgt::Entity player         = Lgt::Entity::Null();
    glm::mat4   camera         = glm::mat4(1.0);
};

} // namespace Game
