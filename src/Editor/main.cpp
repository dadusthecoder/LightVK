#include "Engine/Core/Application.h"
#include "Engine/Scene/Components.h"
#include "Engine/Scene/SceneSerializer.h"

#include "Editor.h"

class EditorApp : public Lgt::Application {
public:
    Lgt::Editor::Editor editor;

    void OnInit() override {
        EnableUi();
        editor.Init(_world.get());

        // Import and instantiate the test model through the asset pipeline.
        LoadModel("D:/DEV/cpp/LightVK/Assets/Sphere/cube.gltf");

        // Create editor camera
        auto cam = _world->CreateEntity("EditorCamera");
        cam.Add<Lgt::Component::Camera>();
        cam.Get<Lgt::Component::LocalTransform>().position = {0.f, 0.f, 5.f};

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
            auto& reg = _world->Registry();
            auto view = reg.view<Lgt::Component::Camera, Lgt::Component::LocalTransform>();
            for (auto entity : view) {
                auto& transform = view.get<Lgt::Component::LocalTransform>(entity);
                auto& cam = view.get<Lgt::Component::Camera>(entity);

                // Mouse look
                glm::vec2 mouseDelta = _input->GetMouseDelta();
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
                if (_input->IsKeyDown(Lgt::Key::LeftShift)) speed *= 2.0f;

                glm::vec3 right = glm::normalize(glm::cross(cam.front, cam.up));
                
                if (_input->IsKeyDown(Lgt::Key::W)) transform.position += cam.front * speed;
                if (_input->IsKeyDown(Lgt::Key::S)) transform.position -= cam.front * speed;
                if (_input->IsKeyDown(Lgt::Key::A)) transform.position -= right * speed;
                if (_input->IsKeyDown(Lgt::Key::D)) transform.position += right * speed;
                if (_input->IsKeyDown(Lgt::Key::E)) transform.position += cam.up * speed;
                if (_input->IsKeyDown(Lgt::Key::Q)) transform.position -= cam.up * speed;

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
