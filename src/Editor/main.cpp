#include "Engine/Core/Application.h"
#include "Engine/Scene/Components.h"
#include "Engine/Scene/SceneSerializer.h"

#include "Editor.h"

class EditorApp : public Lgt::Application {
public:
    Lgt::Editor::Editor editor;

    void OnInit() override {
        EnableUi();
        editor.Init(world_.get());

        // Load test mesh
        LoadMesh("D:/DEV/cpp/LightVK/Assets/Sphere/cube.gltf");

        // Create editor camera
        auto cam = world_->CreateEntity("EditorCamera");
        cam.Add<Lgt::Component::Camera>();
        cam.Get<Lgt::Component::LocalTransform>().position = {0.f, 0.f, 5.f};

        // Test scene setup
        auto e  = world_->CreateEntity("TestSphere");
        auto e1 = world_->CreateEntity("TestChild1");
        auto e2 = world_->CreateEntity("TestChild2");
        auto e3 = world_->CreateEntity("TestChild3");
        auto e4 = world_->CreateEntity("TestChild4");

        world_->Graph().SetParent(e, e4);
        world_->Graph().SetParent(e, e1);
        world_->Graph().SetParent(e1, e2);
        world_->Graph().SetParent(e2, e3);

        // Save scene
        Lgt::SceneSerializer serializer(world_.get());
        serializer.SerializeBinary("scene.bin");
    }

    float yaw   = -90.0f;
    float pitch = 0.0f;

    void OnUpdate(float dt) override {
        auto context = editor.GetContext();
        
        // Right click to capture mouse and look around
        bool rightMouseDown = input_->IsMouseDown(Lgt::Mouse::Right);

        if (context->isViewportHovered && rightMouseDown) {
            input_->SetCursorCaptured(true);
        } else if (!rightMouseDown) {
            input_->SetCursorCaptured(false);
        }

        if (input_->IsCursorCaptured()) {
            auto& reg = world_->Registry();
            auto view = reg.view<Lgt::Component::Camera, Lgt::Component::LocalTransform>();
            for (auto entity : view) {
                auto& transform = view.get<Lgt::Component::LocalTransform>(entity);
                auto& cam = view.get<Lgt::Component::Camera>(entity);

                // Mouse look
                glm::vec2 mouseDelta = input_->GetMouseDelta();
                yaw   += mouseDelta.x * 0.1f;
                pitch -= mouseDelta.y * 0.1f;
                
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;

                glm::vec3 direction;
                direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                direction.y = sin(glm::radians(pitch));
                direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                cam.front = glm::normalize(direction);

                // Keyboard movement
                float speed = 5.0f * dt;
                if (input_->IsKeyDown(Lgt::Key::LeftShift)) speed *= 2.0f;

                glm::vec3 right = glm::normalize(glm::cross(cam.front, cam.up));
                
                if (input_->IsKeyDown(Lgt::Key::W)) transform.position += cam.front * speed;
                if (input_->IsKeyDown(Lgt::Key::S)) transform.position -= cam.front * speed;
                if (input_->IsKeyDown(Lgt::Key::A)) transform.position -= right * speed;
                if (input_->IsKeyDown(Lgt::Key::D)) transform.position += right * speed;
                if (input_->IsKeyDown(Lgt::Key::E)) transform.position += cam.up * speed;
                if (input_->IsKeyDown(Lgt::Key::Q)) transform.position -= cam.up * speed;

                break; // Only control the first active camera
            }
        }
    }

    void OnDrawUi() override {
        // Pure ImGui widgets — engine handles lifecycle
        editor.Update();
    }

    void OnShutdown() override {
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
