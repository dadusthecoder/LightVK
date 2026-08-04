#include "Engine/Core/LightVK.h"
#include "Engine/Core/Logger.h"
#include "Player.h"

class RuntimeApp : public Lgt::Application {
public:
    void OnInit() override {
        Lgt::SceneSerializer serializer(_world.get());
        if (serializer.DeserializeBinary("scene.bin")) {
            LIGHTVK_INFO("Successfully loaded scene.bin in Runtime!");
        } else {
            LIGHTVK_ERROR("Failed to load scene.bin");
        }

        _player_system.Init(_world.get());
    }

    void OnUpdate(float dt) override {
        _player_system.Update(dt, _input.get(), _world.get());
    }

    // No OnDrawUi() override — zero ImGui overhead in Runtime

private:
    Game::PlayerSystem _player_system;
};

int main() {
    RuntimeApp app;
    app.Init();
    app.Run();
    app.Shutdown();
    return 0;
}
