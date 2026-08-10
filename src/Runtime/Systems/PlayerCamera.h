#include "Engine/Core/LightVK.h"
#include "../Context.h"

namespace Game::System {

class PlayerCamera {
public:
    static void                Init(const ApplicationContext& appContext, PlayerContext& playerContext);
    static void                Shutdown();
    static void                Update(float dt, const ApplicationContext& appContext, PlayerContext& playerContext);
    static constexpr glm::vec3 _camera_offset = glm::vec3(0.0f, 0.0f, 0.3f);
};

} // namespace Game::System