#include "Engine/Core/LightVK.h"
#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Gpu/Context.h"
#include "Systems/System.h"
class RuntimeApp : public Lgt::Application {
public:
    Game::ApplicationContext appContext;
    Game::PlayerContext      playerContext;

    void OnInit() override {
        // Lgt::SceneSerializer serializer(_world.get());
        // if (serializer.DeserializeBinary("scene.bin")) {
        //     LIGHTVK_INFO("Successfully loaded scene.bin in Runtime!");
        // } else {
        //     LIGHTVK_ERROR("Failed to load scene.bin");
        // }

        auto modelView = _world->Registry().view<Lgt::Component::ModelInstance>();
        for (const auto entity : modelView) {
            const auto& instance = modelView.get<Lgt::Component::ModelInstance>(entity);
            _assets->LoadModel(instance.model);
        }

        appContext.world  = _world.get();
        appContext.input  = _input.get();
        appContext.timer  = _timer.get();
        appContext.assets = _assets.get();

        // Create a floor
        auto floor                                           = _world->CreateEntity("Floor");
        floor.Get<Lgt::Component::LocalTransform>().position = {0.f, -5.f, 0.f};
        auto floorId = _assets->LoadModel("D:/DEV/cpp/LightVK/Assets/Tests/test_scale_01.glb");
        if (floorId.IsValid())
            floor.Add<Lgt::Component::ModelInstance>().model = floorId;

        auto& floorRB       = floor.Add<Lgt::Component::RigidBody>();
        floorRB.motionType  = Lgt::Component::MotionType::Static;
        floorRB.restitution = 0.5f;
        floor.Add<Lgt::Component::BoxCollider>(glm::vec3{50.f, 0.1f, 50.f});

        // Create a sphere
        auto sphere                                           = _world->CreateEntity("Sphere");
        sphere.Get<Lgt::Component::LocalTransform>().position = {0.f, 10.f, 0.f};
        auto sphereId = _assets->LoadModel("D:/DEV/cpp/LightVK/Assets/Sphere/Sphere.gltf");
        if (sphereId)
            sphere.Add<Lgt::Component::ModelInstance>().model = sphereId;

        auto& sphereRB       = sphere.Add<Lgt::Component::RigidBody>();
        sphereRB.motionType  = Lgt::Component::MotionType::Dynamic;
        sphereRB.restitution = 0.8f;
        sphere.Add<Lgt::Component::SphereCollider>(0.5f);

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
