#pragma once

namespace Lgt {
class World;
class Entity;
} // namespace  Lgt

namespace Lgt::System {

class Renderer {
public:
    explicit Renderer(World* world);
    void Update();

private:
    void   BuildDrawlist();
    World* world_ = nullptr;
};
} // namespace Lgt::System
