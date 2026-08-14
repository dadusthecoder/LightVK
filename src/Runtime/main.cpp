#include "Engine/Core/LightVK.h"
#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Gpu/Context.h"
#include "Systems/System.h"
class RuntimeApp : public Lgt::Application {
public:
    Game::ApplicationContext appContext;
    Game::PlayerContext      playerContext;

    void OnInit() override {
        appContext.world  = _world.get();
        appContext.input  = _input.get();
        appContext.timer  = _timer.get();
        appContext.assets = _assets.get();

        Lgt::SceneSerializer serializer(_world.get());
        if (serializer.DeserializeBinary("scene.bin")) {
            LIGHTVK_INFO("Successfully loaded scene.bin in Runtime!");
        } else {
            LIGHTVK_ERROR("Failed to load scene.bin");
        }

        // Load models for deserialized entities
        auto modelView = _world->Registry().view<Lgt::Component::ModelInstance>();
        for (const auto entity : modelView) {
            const auto& instance = modelView.get<Lgt::Component::ModelInstance>(entity);
            _assets->LoadModel(instance.model);
        }

        Game::System::All::Init(appContext, playerContext);
    }

    void OnRender() override {
        // later for footages we can offload the recoded passes for the rendering parallely on the gpu

        Lgt::Gpu::DrawList sceneDrawList{};
        auto               view = _world->GetView<Lgt::Component::ModelInstance, Lgt::Component::WorldTransform>();

        // this is a very bad idea  ,but its okay for now
        for (auto [entity, instance, worldTransform] : view.each()) {

            auto* model = _assets->GetGpuModel(instance.model);

            if (!instance.visible)
                continue;

            const auto* gpuModel = _assets->GetGpuModel(instance.model);

            if (gpuModel == nullptr)
                continue;

            const size_t firstCommand = sceneDrawList.commands.size();
            sceneDrawList.commands.insert(
                sceneDrawList.commands.end(), gpuModel->drawList.commands.begin(), gpuModel->drawList.commands.end());
            sceneDrawList.indexCounts.insert(
                sceneDrawList.indexCounts.end(), gpuModel->drawList.indexCounts.begin(), gpuModel->drawList.indexCounts.end());
            for (size_t i = firstCommand; i < sceneDrawList.commands.size(); ++i)
                sceneDrawList.commands[i].transform = worldTransform.matrix * sceneDrawList.commands[i].transform;
        }

        if (!sceneDrawList.empty())
            Lgt::Gpu::Renderer->Render(sceneDrawList, playerContext.camera);
    }

    void OnUpdate(float dt) override { Game::System::All::Update(_timer->DeltaTime(), appContext, playerContext); }
};

int main() {
    RuntimeApp app;
    app.Init();
    app.Run();
    app.Shutdown();
    return 0;
}
