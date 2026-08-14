#include "Engine/Core/Application.h"
#include "Engine/Core/Timer.h"
#include "Engine/Scene/World.h"
#include "Engine/Scene/Components.h"
#include "Engine/Core/InputManager.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/Profiler.h"
#include <GLFW/glfw3.h>
#include "Engine/Gpu/Vulkan/Helpers.h"
#include "Engine/Gpu/Vulkan/Context.h"
#include "Engine/Gpu/Context.h"
#include "Engine/UI/ImGuiLayer.h"
#include "Engine/Core/Math.h"

namespace Lgt {

void Application::Init() {

    LOG_INIT();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(WIDTH, HEIGHT, "LightVK Engine", nullptr, nullptr);

    Vulkan::Init(_window);
    Gpu::Init(_window);

    _timer  = std::make_unique<Timer>();
    _world  = std::make_unique<World>();
    _input  = std::make_unique<InputManager>(_window);
    _assets = std::make_unique<Assets::AssetManager>();
    _assets->Init();

    // ImGui is always initialized (it lives in the engine),
    // but the UI pass is only executed if EnableUi() was called.

    _imguiLayer = std::make_unique<ImGuiLayer>();
    _imguiLayer->Init(_window, Gpu::Renderer->SwapchainFormat());

    OnInit();
}

void Application::Run() {
    uint32_t currentFrame = 0;

    while (!glfwWindowShouldClose(_window)) {
        Profiler::BeginFrame();

        _timer->Tick();
        _input->ResetFrame();

        {
            LGT_PROFILE_SCOPE("WorldUpdate");
            _world->Update(_timer->DeltaTime());
        }

        // App logic hook — pure game logic, no rendering
        {
            LGT_PROFILE_SCOPE("OnUpdate");
            OnUpdate(_timer->DeltaTime());
        }

        if (Gpu::Renderer->BeginFrame(currentFrame)) {

            // UI pass — only if enabled (Editor). Zero overhead for Runtime.
            if (uiEnabled_) {
                LGT_PROFILE_SCOPE("UIPass");
                BeginUi();
                OnDrawUi();
                EndUi();
            }

            // Scene pass — find active camera, render all loaded meshes
            {
                if (!uiEnabled_) {
                    Gpu::Renderer->SetRenderToSwapchain(true);
                }
                LGT_PROFILE_SCOPE("RenderScene");
                OnRender();
            }

            Gpu::Renderer->EndFrame();
        }

        glfwPollEvents();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        Profiler::EndFrame();
    }
}

void Application::Shutdown() {
    OnShutdown();
    _assets->Shutdown();
    Gpu::Shutdown();
    _imguiLayer->Shutdown();
    Vulkan::Shutdown();
}

void Application::BeginUi() {
    auto uiCmd = Gpu::Renderer->GetUICommandBuffer();
    _imguiLayer->BeginFrame();
    Gpu::Renderer->BeginRendering(uiCmd, false);
}

void Application::EndUi() {
    auto uiCmd = Gpu::Renderer->GetUICommandBuffer();
    _imguiLayer->EndFrame(uiCmd);
    Gpu::Renderer->EndRendering(uiCmd);
}

Assets::AssetGuid Application::LoadModel(const std::filesystem::path& path) {
    const auto id = _assets->LoadModel(path);
    if (!id.IsValid())
        return id;

    auto entity                                  = _world->CreateEntity(path.stem().string());
    entity.Add<Component::ModelInstance>().model = id;
    return id;
}

Application::Application()  = default;
Application::~Application() = default;

} // namespace Lgt
