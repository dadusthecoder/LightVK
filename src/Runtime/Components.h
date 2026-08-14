#pragma once
#include "Engine/Core/LightVK.h"

namespace Game::Component {
    
struct Interactabel {
    /* data */
};

struct Player {
    std::string name;
    uint64_t    health;
    bool        isAlive;
};

} // namespace Game::Component