#include "Editor.h"
#include <imgui.h>

namespace Lgt {

namespace Editor {

void Editor::Init(World* world) {
    _context.world = world;
    _panel_hirearchy.Init(&_context);
    _panel_viewport.Init(&_context);
    _panel_node_editor.Init(&_context);
    _panel_inspector.Init(&_context);
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

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Editor panels
    _panel_hirearchy.Draw();
    _panel_viewport.Draw();
    _panel_node_editor.Draw();
    _panel_inspector.Draw();

    ImGui::End();
}

void Editor::Shutdown() {
    _panel_inspector.Shutdown();
    _panel_node_editor.Shutdown();
    _panel_viewport.Shutdown();
    _panel_hirearchy.Shutdown();
}
} // namespace Editor

} // namespace Lgt
