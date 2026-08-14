#include "Engine/Core/Application.h"
#include "Engine/Scene/Components.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Physics/PhysicsComponents.h"
#include "Engine/Renderer/Gpu/Context.h"
#include "Utils/FileDialogs.h"

#include "Editor.h"

class EditorApp : public Lgt::Application {
public:
    Lgt::Editor::Editor editor;

    void OnInit() override {
        EnableUi();
        Lgt::Gpu::Renderer->ResizeSceneTarget({1280, 720});

        Lgt::g_WindowHandle = _window;

        editor.Init(_world.get());

        // Editor starts in Edit mode — physics disabled
        _world->SetPhysicsEnabled(false);

        // Wire up callbacks
        auto context    = editor.GetContext();
        context->onPlay = [this]() { EnterPlayMode(); };
        context->onStop = [this]() { ExitPlayMode(); };

        context->onSaveSceneAs = [this, context]() {
            std::string filepath = Lgt::FileDialogs::SaveFile("LightVK Scene (*.bin)\0*.bin\0");
            if (!filepath.empty()) {
                context->currentScenePath = filepath;
                Lgt::SceneSerializer(_world.get()).SerializeBinary(filepath);
            }
        };

        context->onSaveScene = [this, context]() {
            if (!context->currentScenePath.empty()) {
                Lgt::SceneSerializer(_world.get()).SerializeBinary(context->currentScenePath);
            } else {
                if (context->onSaveSceneAs)
                    context->onSaveSceneAs();
            }
        };

        context->onLoadScene = [this, context]() {
            std::string filepath = Lgt::FileDialogs::OpenFile("LightVK Scene (*.bin)\0*.bin\0");
            if (!filepath.empty()) {
                context->currentScenePath = filepath;
                Lgt::SceneSerializer(_world.get()).DeserializeBinary(filepath);

                // Load models for deserialized entities
                auto modelView = _world->Registry().view<Lgt::Component::ModelInstance>();
                for (const auto entity : modelView) {
                    const auto& instance = modelView.get<Lgt::Component::ModelInstance>(entity);
                    _assets->LoadModel(instance.model);
                }
            }
        };

        // Create editor camera
        auto cam = _world->CreateEntity("EditorCamera");
        cam.Add<Lgt::Component::Camera>();
        cam.Get<Lgt::Component::LocalTransform>().position = {0.f, 0.f, 5.f};
    }

    void EnterPlayMode() {
        auto context = editor.GetContext();
        if (context->mode == Lgt::Editor::EditorMode::Play)
            return;

        // Snapshot all LocalTransforms
        context->transformSnapshot.clear();
        auto view = _world->Registry().view<Lgt::Component::LocalTransform>();
        for (auto e : view) {
            auto& lt                                             = view.get<Lgt::Component::LocalTransform>(e);
            context->transformSnapshot[static_cast<uint32_t>(e)] = lt;
        }

        // Enable physics and enter play mode
        _world->SetPhysicsEnabled(true);
        context->mode = Lgt::Editor::EditorMode::Play;
    }

    void ExitPlayMode() {
        auto context = editor.GetContext();
        if (context->mode == Lgt::Editor::EditorMode::Edit)
            return;

        // Remove all physics bodies by resetting _registered flags
        // Physics won't step anymore, so we just need to clean up Jolt state
        auto rbView = _world->Registry().view<Lgt::Component::RigidBody>();
        for (auto e : rbView) {
            Lgt::Entity entity(e, _world.get());
            _world->GetPhysics().RemoveBody(entity);
        }

        // Restore all LocalTransforms from snapshot
        for (auto& [entityId, snapshot] : context->transformSnapshot) {
            auto handle = static_cast<entt::entity>(entityId);
            if (_world->Registry().valid(handle) && _world->Registry().all_of<Lgt::Component::LocalTransform>(handle)) {
                _world->Registry().get<Lgt::Component::LocalTransform>(handle) = snapshot;
            }
        }
        context->transformSnapshot.clear();

        // Disable physics and return to edit mode
        _world->SetPhysicsEnabled(false);
        context->mode = Lgt::Editor::EditorMode::Edit;
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
        } else {
            // Cursor not captured: handle Gizmo shortcuts
            if (_input->WasKeyPressed(Lgt::Key::W))
                context->currentGizmoOperation = Lgt::Editor::GizmoOperation::Translate;
            if (_input->WasKeyPressed(Lgt::Key::E))
                context->currentGizmoOperation = Lgt::Editor::GizmoOperation::Rotate;
            if (_input->WasKeyPressed(Lgt::Key::R))
                context->currentGizmoOperation = Lgt::Editor::GizmoOperation::Scale;
        }

        // Apply transform changes (always needed for editor camera etc.)
        _world->UpdateTransforms();
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

        // Always call Render, even if the draw list is empty,
        // to ensure the scene target is cleared and transitioned properly for ImGui.
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
