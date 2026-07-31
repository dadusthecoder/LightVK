#include "Engine/Core/Application.h"
#include "Engine/Core/Timer.h"
#include "Engine/Scene/World.h"
#include "Engine/Scene/Components.h"
#include "Engine/Core/InputManager.h"
#include "Engine/Core/Logger.h"
#include <GLFW/glfw3.h>
#include "Engine/Renderer/Vulkan/Helpers.h"
#include "Engine/Renderer/Vulkan/Context.h"
#include "Engine/Renderer/Gpu/Context.h"
#include "Engine/Renderer/Gpu/Resource.h"
#include "Engine/UI/ImGuiLayer.h"
#include "Engine/Core/Math.h"

// Asset loading (will be moved to Engine later)
#include "Engine/Assets/Assets.h"

namespace Lgt {

void Application::Init() {

    LOG_INIT();

    LIGHTVK_CRITICAL("Testing Critical");
    LIGHTVK_ERROR("Testing Error");
    LIGHTVK_TRACE("Testing Trace");
    LIGHTVK_WARN("Testing Warn");
    LIGHTVK_INFO("Testing Info");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window_ = glfwCreateWindow(WIDTH, HEIGHT, "LightVK Engine", nullptr, nullptr);

    Vulkan::Init(window_);
    Gpu::Init(window_);

    timer_ = std::make_unique<Timer>();
    world_ = std::make_unique<World>();
    input_ = std::make_unique<InputManager>(window_);

    // ImGui is always initialized (it lives in the engine),
    // but the UI pass is only executed if EnableUi() was called.
    imguiLayer_ = std::make_unique<ImGuiLayer>();
    imguiLayer_->Init(window_, Gpu::Renderer->SwapchainFormat());

    OnInit();
}

void Application::Run() {
    uint32_t currentFrame = 0;

    while (!glfwWindowShouldClose(window_)) {

        timer_->Tick();
        input_->ResetFrame();
        world_->Update(timer_->DeltaTime());

        // App logic hook — pure game logic, no rendering
        OnUpdate(timer_->DeltaTime());

        if (Gpu::Renderer->BeginFrame(currentFrame)) {

            // UI pass — only if enabled (Editor). Zero overhead for Runtime.
            if (uiEnabled_) {
                BeginUi();
                OnDrawUi();
                EndUi();
            }

            // Scene pass — find active camera, render all loaded meshes
            RenderScene();

            Gpu::Renderer->EndFrame();
        }

        glfwPollEvents();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
}

void Application::Shutdown() {
    OnShutdown();
    imguiLayer_->Shutdown();
    Gpu::Shutdown();
    Vulkan::Shutdown();
}

void Application::BeginUi() {
    auto uiCmd = Gpu::Renderer->GetUICommandBuffer();
    imguiLayer_->BeginFrame();
    Gpu::Renderer->BeginRendering(uiCmd, false);
}

void Application::EndUi() {
    auto uiCmd = Gpu::Renderer->GetUICommandBuffer();
    imguiLayer_->EndFrame(uiCmd);
    Gpu::Renderer->EndRendering(uiCmd);
}

void Application::RenderScene() {
    // Find the active camera in the ECS
    glm::mat4 viewProj = glm::mat4(1.0f); // identity fallback

    auto& reg = world_->Registry();
    auto  view = reg.view<Component::Camera, Component::LocalTransform>();

    for (auto entity : view) {
        auto& cam       = view.get<Component::Camera>(entity);
        auto& transform = view.get<Component::LocalTransform>(entity);

        if (!cam.isActive)
            continue;

        // Use offscreen target dimensions if valid, else fallback to Swapchain (runtime)
        uint32_t sceneW = Gpu::Renderer->GetSceneWidth();
        uint32_t sceneH = Gpu::Renderer->GetSceneHeight();
        if (sceneW == 0 || sceneH == 0) {
            sceneW = WIDTH;
            sceneH = HEIGHT;
        }
        float aspect = static_cast<float>(sceneW) / static_cast<float>(sceneH);

        glm::mat4 viewMat = cam.ViewMatrix(transform.position);
        glm::mat4 projMat = cam.ProjectionMatrix(aspect);
        viewProj = projMat * viewMat;
        break; // Use first active camera
    }

    // Render all loaded meshes
    for (auto& mesh : meshes_) {
        Gpu::Renderer->Render(&mesh, viewProj);
    }
}

Gpu::DrawList Application::LoadMesh(const std::filesystem::path& path) {
    Assets::Model model;
    Assets::LoadGltf(path, &model);

    Gpu::DrawCommand* commands    = new Gpu::DrawCommand[model.meshes.size()];
    uint32_t*         indexCounts = new uint32_t[model.meshes.size()];

    for (unsigned int i = 0; i < model.meshes.size(); ++i) {
        auto vbo = Gpu::Resources->CreateBuffer(Gpu::BufferDesc::SSBO(model.meshes[i].vertices.size() * sizeof(Gpu::Vertex)));
        auto ibo = Gpu::Resources->CreateBuffer(Gpu::BufferDesc::SSBO(model.meshes[i].indices.size() * sizeof(uint32_t)));

        Vulkan::g_Uploader->UploadBuffer(Gpu::Resources->GetBuffer(vbo)->buffer,
                                                  model.meshes[i].vertices.data(),
                                                  model.meshes[i].vertices.size() * sizeof(Gpu::Vertex));

        Vulkan::g_Uploader->UploadBuffer(Gpu::Resources->GetBuffer(ibo)->buffer,
                                                  model.meshes[i].indices.data(),
                                                  model.meshes[i].indices.size() * sizeof(uint32_t));

        commands[i].vertexBufferIndex = Gpu::ResourceHeap->AllocateSSBO(vbo);
        commands[i].indexBufferIndex  = Gpu::ResourceHeap->AllocateSSBO(ibo);
        indexCounts[i]                = model.meshes[i].indices.size();
    }

    Gpu::DrawList drawList{};
    drawList.commands    = commands;
    drawList.count       = model.meshes.size();
    drawList.indexCounts = indexCounts;
    Vulkan::g_Uploader->Flush();

    // Store internally for RenderScene()
    meshes_.push_back(drawList);

    return drawList;
}

Application::Application()  = default;
Application::~Application() = default;

} // namespace Lgt
