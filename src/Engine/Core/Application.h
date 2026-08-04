#pragma once
#include <memory>
#include <vector>
#include <filesystem>

#include "Engine/Core/Timer.h"
#include "Engine/Scene/World.h"
#include "Engine/Core/InputManager.h"
#include "Engine/Renderer/Gpu/Renderer.h"

struct GLFWwindow;

namespace Lgt {

class ImGuiLayer;

class Application {
public:
    Application();
    virtual ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Init();
    void Run();
    void Shutdown();

protected:
    /// Override for game logic (input, physics, AI, etc.)
    virtual void OnInit() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnShutdown() {}

    /// Override to draw ImGui widgets. Only called if UI is enabled.
    /// The engine handles BeginFrame/EndFrame and ImGui lifecycle automatically.
    virtual void OnDrawUi() {}

    /// Call in OnInit() to enable the ImGui UI pass.
    /// Runtime apps should NOT call this — zero ImGui overhead.
    void EnableUi() { uiEnabled_ = true; }

    /// Load a mesh from disk. Returns a handle for future reference.
    /// Wraps all GPU internals (SSBO creation, upload, descriptor allocation).
    Gpu::DrawList LoadMesh(const std::filesystem::path& path);

    GLFWwindow*                   _window = nullptr;
    std::unique_ptr<World>        _world;
    std::unique_ptr<InputManager> _input;
    std::unique_ptr<Timer>        _timer;

private:
    void BeginUi();
    void EndUi();
    void RenderScene();

    std::unique_ptr<ImGuiLayer>  _imguiLayer;
    std::vector<Gpu::DrawList>   _meshes;
    bool                         uiEnabled_ = false;
};

} // namespace Lgt
