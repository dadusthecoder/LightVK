#include "Editor.h"
#include <imgui.h>

namespace Lgt {

namespace Editor {

void Editor::Init(World* world) {
    context_.world = world;
    panel_hirearchy_.Init(&context_);
    panel_viewport_.Init(&context_);
    panel_node_editor_.Init(&context_);
    panel_inspector_.Init(&context_);
}

void Editor::Update() {
    // Create full-screen dockspace
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Editor panels
    panel_hirearchy_.Draw();
    panel_viewport_.Draw();
    panel_node_editor_.Draw();
    panel_inspector_.Draw();
    ImGui::ShowDemoWindow();

    ImGui::End();
}

void Editor::Shutdown() {
    panel_inspector_.Shutdown();
    panel_node_editor_.Shutdown();
    panel_viewport_.Shutdown();
    panel_hirearchy_.Shutdown();
}
} // namespace Editor

} // namespace Lgt
