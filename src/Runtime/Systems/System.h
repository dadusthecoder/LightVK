#include "Engine/Core/LightVK.h"
#include "Player.h"
#include "PlayerCamera.h"

namespace Game {

namespace System {

template <typename... BaseSystems> class Systems {
public:
    static void Init(const ApplicationContext& appContext, PlayerContext& playerContext) { (BaseSystems::Init(appContext, playerContext), ...); }
    static void Update(float dt, const ApplicationContext& appContext, PlayerContext& playerContext) { (BaseSystems::Update(dt, appContext, playerContext), ...); }
};

using All = Systems<Player, PlayerCamera>;

} // namespace System

// rules to keep in mind while desiginig a system
//  1) systems have no state or data
//  2)

} // namespace Game
