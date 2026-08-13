#include "Engine/Core/Application.h"
#include "Engine/Scene/Components.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Physics/PhysicsComponents.h"
#include "Engine/Renderer/Gpu/Context.h"

#include "Editor.h"

class EditorApp : public Lgt::Application {
public:
    Lgt::Editor::Editor editor;

    void OnInit() override {
        EnableUi();
        Lgt::Gpu::Renderer->ResizeSceneTarget({1280, 720});
        editor.Init(_world.get());

        // Create editor camera
        auto cam = _world->CreateEntity("EditorCamera");
        cam.Add<Lgt::Component::Camera>();
        cam.Get<Lgt::Component::LocalTransform>().position = {0.f, 0.f, 5.f};

        // Create a floor
        auto floor                                           = _world->CreateEntity("Floor");
        floor.Get<Lgt::Component::LocalTransform>().position = {0.f, -5.f, 0.f};
        floor.Get<Lgt::Component::LocalTransform>().scale    = {50.f, 0.1f, 50.f};
        auto floorId                                         = _assets->LoadModel("D:/DEV/cpp/LightVK/Assets/cube/cube.gltf");
        if (floorId.IsValid())
            floor.Add<Lgt::Component::ModelInstance>().model = floorId;

        auto& floorRB       = floor.Add<Lgt::Component::RigidBody>();
        floorRB.motionType  = Lgt::Component::MotionType::Static;
        floorRB.restitution = 0.5f;
        floor.Add<Lgt::Component::BoxCollider>(glm::vec3{50.f, 0.5f, 50.f});

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
        
    }

    float yaw   = -90.0f;
    float pitch = 0.0f;

    void OnUpdate(float dt) override {
        auto context = editor.GetContext();

        // Right click to capture mouse and look around
        bool rightMouseDown = _input->IsMouseDown(Lgt::Mouse::Right);

        if (context->isViewportHovered && rightMouseDown) {
            _input->SetCursorCaptured(true);
        } else if (!rightMouseDown) {
            _input->SetCursorCaptured(false);
        }

        if (_input->IsCursorCaptured()) {
            auto& reg  = _world->Registry();
            auto  view = reg.view<Lgt::Component::Camera, Lgt::Component::LocalTransform>();
            for (auto entity : view) {
                auto& transform = view.get<Lgt::Component::LocalTransform>(entity);
                auto& cam       = view.get<Lgt::Component::Camera>(entity);

                // Mouse look
                glm::vec2 mouseDelta  = _input->GetMouseDelta();
                yaw                  += mouseDelta.x * 0.1f;
                pitch                -= mouseDelta.y * 0.1f;

                if (pitch > 89.0f)
                    pitch = 89.0f;
                if (pitch < -89.0f)
                    pitch = -89.0f;

                glm::vec3 direction;
                direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                direction.y = sin(glm::radians(pitch));
                direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                cam.front   = glm::normalize(direction);

                // Keyboard movement
                float speed = 5.0f * dt;
                if (_input->IsKeyDown(Lgt::Key::LeftShift))
                    speed *= 2.0f;

                glm::vec3 right = glm::normalize(glm::cross(cam.front, cam.up));

                if (_input->IsKeyDown(Lgt::Key::W))
                    transform.position += cam.front * speed;
                if (_input->IsKeyDown(Lgt::Key::S))
                    transform.position -= cam.front * speed;
                if (_input->IsKeyDown(Lgt::Key::A))
                    transform.position -= right * speed;
                if (_input->IsKeyDown(Lgt::Key::D))
                    transform.position += right * speed;
                if (_input->IsKeyDown(Lgt::Key::E))
                    transform.position += cam.up * speed;
                if (_input->IsKeyDown(Lgt::Key::Q))
                    transform.position -= cam.up * speed;

                break; // Only control the first active camera
            }
        }
    }

    void OnRender() override {
        // Find the active camera in the ECS
        glm::mat4 viewProj = glm::mat4(1.0f); // identity fallback

        auto& reg  = _world->Registry();
        auto  view = reg.view<Lgt::Component::Camera, Lgt::Component::LocalTransform>();

        for (auto entity : view) {
            auto& cam       = view.get<Lgt::Component::Camera>(entity);
            auto& transform = view.get<Lgt::Component::LocalTransform>(entity);

            if (!cam.isActive)
                continue;

            // Use offscreen target dimensions if valid, else fallback to Swapchain (runtime)
            uint32_t sceneW = Lgt::Gpu::Renderer->GetCurrentExtent().width;
            uint32_t sceneH = Lgt::Gpu::Renderer->GetCurrentExtent().height;
            if (sceneW == 0 || sceneH == 0) {
                sceneW = WIDTH;
                sceneH = HEIGHT;
            }
            float aspect = static_cast<float>(sceneW) / static_cast<float>(sceneH);

            glm::mat4 viewMat = cam.ViewMatrix(transform.position);
            glm::mat4 projMat = cam.ProjectionMatrix(aspect);
            viewProj          = projMat * viewMat;
            break; // Use first active camera
        }

        Lgt::Gpu::DrawList sceneDrawList;
        auto               modelView = _world->Registry().view<Lgt::Component::ModelInstance, Lgt::Component::WorldTransform>();
        for (const auto entity : modelView) {

                const auto& worldTransform = modelView.get<Lgt::Component::WorldTransform>(entity).matrix;
                const auto& instance       = modelView.get<Lgt::Component::ModelInstance>(entity);

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
                    sceneDrawList.commands[i].transform = worldTransform * sceneDrawList.commands[i].transform;
            }

        if (!sceneDrawList.empty())
            Lgt::Gpu::Renderer->Render(sceneDrawList, viewProj);
    }

    void OnDrawUi() override {
        // Pure ImGui widgets — engine handles lifecycle
        editor.Update();
    }

    void OnShutdown() override {
        Lgt::SceneSerializer(_world.get()).SerializeBinary("scene.bin");
        editor.Shutdown();
    }
};

int main() {
    EditorApp app;
    app.Init();
    app.Run();
    app.Shutdown();
    return 0;
}
