#pragma once
#include "Engine/Core/LightVK.h"

#include "../Components.h"
#include "../Context.h"

namespace Game::System {

class Player {
    public:
    static void Init(const ApplicationContext& appContext, PlayerContext& playerContext);
    static void Update(float dt, const ApplicationContext& appContext, PlayerContext& playerContext);
    static void ShutDown();

private:
};
} // namespace Game::System
