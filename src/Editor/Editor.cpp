#include "Editor.h"
#include <imgui.h>
#include <ImGuizmo.h>

namespace Lgt {

namespace Editor {

void Editor::Init(World* world) {
    _context.world = world;
    _panel_hirearchy.Init(&_context);
    _panel_viewport.Init(&_context);
    _panel_node_editor.Init(&_context);
    _panel_inspector.Init(&_context);
    _panel_console.Init(&_context);
    _panel_profiler.Init(&_context);
#if defined(LIGHTVK_EDITOR_TESTS)
    _test_runner.Init(world);
#endif
}

void Editor::Update() {
    ImGuizmo::BeginFrame();

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

    // ── Play/Stop Toolbar ──────────────────────────────────────────────
    {
        float toolbarHeight = 32.0f;
        ImVec2 toolbarSize = ImVec2(ImGui::GetContentRegionAvail().x, toolbarHeight);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::BeginChild("##Toolbar", toolbarSize, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
        
        float buttonWidth = 60.0f;
        float totalWidth = buttonWidth * 2 + 8.0f; // two buttons + spacing
        float startX = (toolbarSize.x - totalWidth) * 0.5f;
        ImGui::SetCursorPosX(startX);
        ImGui::SetCursorPosY((toolbarHeight - ImGui::GetFontSize() - ImGui::GetStyle().FramePadding.y * 2.0f) * 0.5f);

        bool isPlaying = (_context.mode == EditorMode::Play);

        if (!isPlaying) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
            if (ImGui::Button("Play", ImVec2(buttonWidth, 0))) {
                if (_context.onPlay) _context.onPlay();
            }
            ImGui::PopStyleColor(3);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("Stop", ImVec2(buttonWidth, 0))) {
                if (_context.onStop) _context.onStop();
            }
            ImGui::PopStyleColor(3);
        }
        
        // Show mode indicator
        ImGui::SameLine();
        ImGui::SetCursorPosY((toolbarHeight - ImGui::GetFontSize()) * 0.5f);
        if (isPlaying) {
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Playing");
        } else {
            ImGui::TextDisabled("Edit Mode");
        }
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Editor panels
    _panel_hirearchy.Draw();
    _panel_viewport.Draw();
    _panel_node_editor.Draw();
    _panel_inspector.Draw();
    _panel_console.Draw();
    _panel_profiler.Draw();

#if defined(LIGHTVK_EDITOR_TESTS)
    _test_runner.DrawUi();
#endif

    ImGui::End();
}

void Editor::Shutdown() {
    _panel_profiler.Shutdown();
    _panel_console.Shutdown();
    _panel_inspector.Shutdown();
    _panel_node_editor.Shutdown();
    _panel_viewport.Shutdown();
    _panel_hirearchy.Shutdown();
#if defined(LIGHTVK_EDITOR_TESTS)
    _test_runner.Shutdown();
#endif
}
} // namespace Editor

} // namespace Lgt
