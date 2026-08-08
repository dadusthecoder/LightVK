#include "Engine/Core/LightVK.h"

namespace Game {

namespace Components {
struct Player {
    std::string name;
    uint64_t    health;
    bool        isAlive; 
};
} // namespace  Components

class PlayerSystem {
public:
    void Update(float dt, Lgt::InputManager* input, Lgt::World* world);
    void Init(Lgt::World* world);
    void ShutDown();

private:
    Lgt::Entity player;
};
} // namespace Game
