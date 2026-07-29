#pragma once
#include "Engine/Scene/Entity.h"

namespace Lgt {

namespace Editor {

struct Context {
    World* world          = nullptr;
    Entity selectedEntity = Entity::Null();

    bool isViewportHovered = false;
    bool isViewportFocused = false;
};

} // namespace Editor

} // namespace Lgt