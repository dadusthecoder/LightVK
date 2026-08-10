#pragma once
#include <memory>
#include <vector>
#include <filesystem>

#include "Engine/Core/Timer.h"
#include "Engine/Scene/World.h"
#include "Engine/Core/InputManager.h"
#include "Engine/Renderer/Gpu/Renderer.h"
#include "Engine/Assets/AssetManager.h"


struct GLFWwindow;

namespace Lgt {

class ImGuiLayer;

class Application {
public:
    Application();
    virtual ~Application();
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    void Init();
    void Run();
    void Shutdown();

protected:
    /// Override for game logic (input, physics, AI, etc.)
    virtual void OnInit() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnShutdown() {}

    // called between the BeginFrame and EndFrame
    // use LgT::Gpu::Renderer->Render(drawlst , viewproj) to render the scene
    virtual void OnRender() {}

    /// Override to draw ImGui widgets. Only called if UI is enabled.
    /// The engine handles BeginFrame/EndFrame and ImGui lifecycle automatically.
    virtual void OnDrawUi() {}

    /// Call in OnInit() to enable the ImGui UI pass.
    /// Runtime apps should NOT call this — zero ImGui overhead.
    void EnableUi() { uiEnabled_ = true; }

    /// Import, upload, and instantiate a model in the current world.
    Assets::AssetGuid LoadModel(const std::filesystem::path& path);

    GLFWwindow*                           _window = nullptr;
    std::unique_ptr<World>                _world;
    std::unique_ptr<InputManager>         _input;
    std::unique_ptr<Timer>                _timer;
    std::unique_ptr<Assets::AssetManager> _assets;

private:
    void BeginUi();
    void EndUi();

    std::unique_ptr<ImGuiLayer> _imguiLayer;
    bool                        uiEnabled_ = false;
};

} // namespace Lgt
